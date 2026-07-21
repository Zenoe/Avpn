#include "spa/gmSpaCodec.h"

#include <QJsonDocument>
#include <QJsonObject>

#ifdef SPA_DEBUG_LOGGING
  #include <QDebug>
#endif

#include <array>
#include <cstring>
#include <limits>
#include <memory>

#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/crypto.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>
#include <openssl/params.h>
#include <openssl/rand.h>

#include "spaCrypto.h"

namespace {

constexpr char kMagic[] = "ASPA";
constexpr quint8 kRequestType = 0x01;
constexpr quint8 kResponseType = 0x02;
constexpr int kRequestIdSize = 32;
constexpr int kPublicKeySize = 65;
constexpr int kGcmNonceSize = 12;
constexpr int kSm4KeySize = 16;
constexpr int kGcmTagSize = 16;
constexpr int kResponseEncryptionKeyIdOffset = 6;
constexpr int kResponseSigningKeyIdOffset = kResponseEncryptionKeyIdOffset + 2;
constexpr int kResponseRequestIdOffset = kResponseSigningKeyIdOffset + 2;
constexpr int kResponseGcmNonceOffset = kResponseRequestIdOffset + kRequestIdSize;
constexpr int kResponseCiphertextLengthOffset = kResponseGcmNonceOffset + kGcmNonceSize;
constexpr int kResponsePrefixSize = kResponseCiphertextLengthOffset + 2;
constexpr int kMaximumDatagramSize = 65507;

using EcKeyPtr = std::unique_ptr<EC_KEY, decltype(&EC_KEY_free)>;
using EcPointPtr = std::unique_ptr<EC_POINT, decltype(&EC_POINT_free)>;
using BnContextPtr = std::unique_ptr<BN_CTX, decltype(&BN_CTX_free)>;
using BigNumberPtr = std::unique_ptr<BIGNUM, decltype(&BN_free)>;
using CipherContextPtr = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;
using PkeyContextPtr = std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>;
using PkeyPtr = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
using MdContextPtr = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

QString opensslError(const QString &prefix)
{
    const unsigned long code = ERR_get_error();
    if (code == 0) {
        return prefix;
    }
    std::array<char, 256> text {};
    ERR_error_string_n(code, text.data(), text.size());
    return prefix + QStringLiteral(": ") + QString::fromLatin1(text.data());
}

QByteArray randomBytes(int size)
{
    if (size <= 0) {
        return {};
    }
    QByteArray result(size, Qt::Uninitialized);
    if (RAND_bytes(reinterpret_cast<unsigned char *>(result.data()), size) != 1) {
        return {};
    }
    return result;
}

QByteArray base64Url(const QByteArray &value)
{
    return value.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

void appendUint16(QByteArray &output, quint16 value)
{
    output.append(static_cast<char>((value >> 8) & 0xff));
    output.append(static_cast<char>(value & 0xff));
}

void appendUint64(QByteArray &output, quint64 value)
{
    for (int shift = 56; shift >= 0; shift -= 8) {
        output.append(static_cast<char>((value >> shift) & 0xff));
    }
}

bool readUint16(const QByteArray &input, qsizetype offset, quint16 *value)
{
    if (!value || offset < 0 || offset + 2 > input.size()) {
        return false;
    }
    *value = (static_cast<quint16>(static_cast<quint8>(input.at(offset))) << 8)
            | static_cast<quint8>(input.at(offset + 1));
    return true;
}

QByteArray deriveKey(const QByteArray &label, const QByteArray &sharedSecret, const QByteArray &requestId)
{
    const QByteArray digest = spa::crypto::sm3(label + sharedSecret + requestId);
    return digest.left(kSm4KeySize);
}

bool createSharedSecret(const QByteArray &serverPublicKey, QByteArray *clientPublicKey,
                        QByteArray *sharedSecret, QString *errorMessage)
{
    if (!clientPublicKey || !sharedSecret || serverPublicKey.size() != kPublicKeySize) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid SM2 key input");
        }
        return false;
    }

    EcKeyPtr ephemeralKey(EC_KEY_new_by_curve_name(NID_sm2), EC_KEY_free);
    if (!ephemeralKey || EC_KEY_generate_key(ephemeralKey.get()) != 1) {
        if (errorMessage) {
            *errorMessage = opensslError(QStringLiteral("Unable to generate the ephemeral SM2 key"));
        }
        return false;
    }

    const EC_GROUP *group = EC_KEY_get0_group(ephemeralKey.get());
    const EC_POINT *ephemeralPoint = EC_KEY_get0_public_key(ephemeralKey.get());
    const BIGNUM *ephemeralPrivate = EC_KEY_get0_private_key(ephemeralKey.get());
    if (!group || !ephemeralPoint || !ephemeralPrivate) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Incomplete ephemeral SM2 key");
        }
        return false;
    }

    clientPublicKey->resize(kPublicKeySize);
    if (EC_POINT_point2oct(group, ephemeralPoint, POINT_CONVERSION_UNCOMPRESSED,
                           reinterpret_cast<unsigned char *>(clientPublicKey->data()), clientPublicKey->size(), nullptr)
        != kPublicKeySize) {
        if (errorMessage) {
            *errorMessage = opensslError(QStringLiteral("Unable to encode the ephemeral SM2 public key"));
        }
        return false;
    }

    BnContextPtr bnContext(BN_CTX_new(), BN_CTX_free);
    EcPointPtr serverPoint(EC_POINT_new(group), EC_POINT_free);
    EcPointPtr resultPoint(EC_POINT_new(group), EC_POINT_free);
    BigNumberPtr x(BN_new(), BN_free);
    BigNumberPtr y(BN_new(), BN_free);
    if (!bnContext || !serverPoint || !resultPoint || !x || !y
        || EC_POINT_oct2point(group, serverPoint.get(),
                             reinterpret_cast<const unsigned char *>(serverPublicKey.constData()),
                             serverPublicKey.size(), bnContext.get()) != 1
        || EC_POINT_is_on_curve(group, serverPoint.get(), bnContext.get()) != 1
        || EC_POINT_mul(group, resultPoint.get(), nullptr, serverPoint.get(), ephemeralPrivate, bnContext.get()) != 1
        || EC_POINT_is_at_infinity(group, resultPoint.get()) == 1
        || EC_POINT_get_affine_coordinates(group, resultPoint.get(), x.get(), y.get(), bnContext.get()) != 1) {
        if (errorMessage) {
            *errorMessage = opensslError(QStringLiteral("Unable to derive the SM2 shared point"));
        }
        return false;
    }

    sharedSecret->resize(64);
    if (BN_bn2binpad(x.get(), reinterpret_cast<unsigned char *>(sharedSecret->data()), 32) != 32
        || BN_bn2binpad(y.get(), reinterpret_cast<unsigned char *>(sharedSecret->data()) + 32, 32) != 32) {
        if (errorMessage) {
            *errorMessage = opensslError(QStringLiteral("Unable to encode the SM2 shared point"));
        }
        return false;
    }
    return true;
}

class Sm4BlockCipher
{
public:
    explicit Sm4BlockCipher(const QByteArray &key)
        : m_context(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free)
    {
        m_valid = key.size() == kSm4KeySize && m_context
                && EVP_EncryptInit_ex(m_context.get(), EVP_sm4_ecb(), nullptr,
                                      reinterpret_cast<const unsigned char *>(key.constData()), nullptr) == 1
                && EVP_CIPHER_CTX_set_padding(m_context.get(), 0) == 1;
    }

    bool encryptBlock(const std::array<quint8, 16> &input, std::array<quint8, 16> *output)
    {
        if (!m_valid || !output) {
            return false;
        }
        int outputLength = 0;
        return EVP_EncryptUpdate(m_context.get(), output->data(), &outputLength, input.data(), input.size()) == 1
                && outputLength == 16;
    }

    bool isValid() const { return m_valid; }

private:
    CipherContextPtr m_context;
    bool m_valid = false;
};

void xorBlock(std::array<quint8, 16> *left, const std::array<quint8, 16> &right)
{
    for (size_t i = 0; i < left->size(); ++i) {
        (*left)[i] ^= right[i];
    }
}

void shiftRight(std::array<quint8, 16> *value)
{
    quint8 carry = 0;
    for (size_t i = 0; i < value->size(); ++i) {
        const quint8 nextCarry = static_cast<quint8>((*value)[i] & 1);
        (*value)[i] = static_cast<quint8>(((*value)[i] >> 1) | (carry << 7));
        carry = nextCarry;
    }
}

std::array<quint8, 16> galoisMultiply(const std::array<quint8, 16> &x,
                                      const std::array<quint8, 16> &y)
{
    std::array<quint8, 16> z {};
    std::array<quint8, 16> v = y;
    for (int bit = 0; bit < 128; ++bit) {
        if ((x[bit / 8] & static_cast<quint8>(0x80 >> (bit % 8))) != 0) {
            xorBlock(&z, v);
        }
        const bool leastSignificantBit = (v[15] & 1) != 0;
        shiftRight(&v);
        if (leastSignificantBit) {
            v[0] ^= 0xe1;
        }
    }
    return z;
}

void ghashSection(std::array<quint8, 16> *state, const std::array<quint8, 16> &hashSubkey,
                  const QByteArray &section)
{
    for (qsizetype offset = 0; offset < section.size(); offset += 16) {
        std::array<quint8, 16> block {};
        const qsizetype bytes = qMin<qsizetype>(16, section.size() - offset);
        std::memcpy(block.data(), section.constData() + offset, static_cast<size_t>(bytes));
        xorBlock(state, block);
        *state = galoisMultiply(*state, hashSubkey);
    }
}

void incrementCounter(std::array<quint8, 16> *counter)
{
    for (int i = 15; i >= 12; --i) {
        (*counter)[i] = static_cast<quint8>((*counter)[i] + 1);
        if ((*counter)[i] != 0) {
            break;
        }
    }
}

bool sm4GcmCrypt(const QByteArray &key, const QByteArray &nonce, const QByteArray &aad,
                 const QByteArray &input, QByteArray *output, QByteArray *tag, QString *errorMessage)
{
    if (!output || !tag || key.size() != kSm4KeySize || nonce.size() != kGcmNonceSize) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid SM4-GCM input");
        }
        return false;
    }

    Sm4BlockCipher cipher(key);
    std::array<quint8, 16> zero {};
    std::array<quint8, 16> hashSubkey {};
    if (!cipher.isValid() || !cipher.encryptBlock(zero, &hashSubkey)) {
        if (errorMessage) {
            *errorMessage = opensslError(QStringLiteral("Unable to initialize SM4"));
        }
        return false;
    }

    std::array<quint8, 16> initialCounter {};
    std::memcpy(initialCounter.data(), nonce.constData(), kGcmNonceSize);
    initialCounter[15] = 1;

    output->resize(input.size());
    std::array<quint8, 16> counter = initialCounter;
    for (qsizetype offset = 0; offset < input.size(); offset += 16) {
        incrementCounter(&counter);
        std::array<quint8, 16> stream {};
        if (!cipher.encryptBlock(counter, &stream)) {
            if (errorMessage) {
                *errorMessage = opensslError(QStringLiteral("SM4 counter encryption failed"));
            }
            return false;
        }
        const qsizetype bytes = qMin<qsizetype>(16, input.size() - offset);
        for (qsizetype i = 0; i < bytes; ++i) {
            (*output)[offset + i] = static_cast<char>(static_cast<quint8>(input.at(offset + i))
                                                      ^ stream[static_cast<size_t>(i)]);
        }
    }

    std::array<quint8, 16> authenticationState {};
    ghashSection(&authenticationState, hashSubkey, aad);
    ghashSection(&authenticationState, hashSubkey, *output);

    QByteArray lengthBlock;
    lengthBlock.reserve(16);
    appendUint64(lengthBlock, static_cast<quint64>(aad.size()) * 8);
    appendUint64(lengthBlock, static_cast<quint64>(output->size()) * 8);
    std::array<quint8, 16> lengths {};
    std::memcpy(lengths.data(), lengthBlock.constData(), 16);
    xorBlock(&authenticationState, lengths);
    authenticationState = galoisMultiply(authenticationState, hashSubkey);

    std::array<quint8, 16> encryptedInitialCounter {};
    if (!cipher.encryptBlock(initialCounter, &encryptedInitialCounter)) {
        if (errorMessage) {
            *errorMessage = opensslError(QStringLiteral("SM4 tag encryption failed"));
        }
        return false;
    }
    xorBlock(&authenticationState, encryptedInitialCounter);
    *tag = QByteArray(reinterpret_cast<const char *>(authenticationState.data()), authenticationState.size());
    return true;
}

bool sm4GcmEncrypt(const QByteArray &key, const QByteArray &nonce, const QByteArray &aad,
                   const QByteArray &plainText, QByteArray *cipherTextWithTag, QString *errorMessage)
{
    QByteArray cipherText;
    QByteArray tag;
    if (!sm4GcmCrypt(key, nonce, aad, plainText, &cipherText, &tag, errorMessage)) {
        return false;
    }
    *cipherTextWithTag = cipherText + tag;
    return true;
}

bool sm4GcmDecrypt(const QByteArray &key, const QByteArray &nonce, const QByteArray &aad,
                   const QByteArray &cipherTextWithTag, QByteArray *plainText, QString *errorMessage)
{
    if (!plainText || key.size() != kSm4KeySize || nonce.size() != kGcmNonceSize
        || cipherTextWithTag.size() < kGcmTagSize) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("SM4-GCM ciphertext is shorter than the authentication tag");
        }
        return false;
    }

    const QByteArray cipherText = cipherTextWithTag.left(cipherTextWithTag.size() - kGcmTagSize);
    const QByteArray receivedTag = cipherTextWithTag.right(kGcmTagSize);
    QByteArray calculatedTag;

    // GCM authentication is calculated over ciphertext. Passing ciphertext through
    // CTR once produces plaintext, while the helper authenticates its output. Run
    // authentication explicitly and then decrypt below.
    Sm4BlockCipher cipher(key);
    std::array<quint8, 16> zero {};
    std::array<quint8, 16> hashSubkey {};
    if (!cipher.isValid() || !cipher.encryptBlock(zero, &hashSubkey)) {
        if (errorMessage) {
            *errorMessage = opensslError(QStringLiteral("Unable to initialize SM4"));
        }
        return false;
    }

    std::array<quint8, 16> initialCounter {};
    std::memcpy(initialCounter.data(), nonce.constData(), kGcmNonceSize);
    initialCounter[15] = 1;
    std::array<quint8, 16> state {};
    ghashSection(&state, hashSubkey, aad);
    ghashSection(&state, hashSubkey, cipherText);
    QByteArray lengthBlock;
    appendUint64(lengthBlock, static_cast<quint64>(aad.size()) * 8);
    appendUint64(lengthBlock, static_cast<quint64>(cipherText.size()) * 8);
    std::array<quint8, 16> lengths {};
    std::memcpy(lengths.data(), lengthBlock.constData(), 16);
    xorBlock(&state, lengths);
    state = galoisMultiply(state, hashSubkey);
    std::array<quint8, 16> encryptedInitialCounter {};
    if (!cipher.encryptBlock(initialCounter, &encryptedInitialCounter)) {
        return false;
    }
    xorBlock(&state, encryptedInitialCounter);
    calculatedTag = QByteArray(reinterpret_cast<const char *>(state.data()), state.size());

    if (CRYPTO_memcmp(receivedTag.constData(), calculatedTag.constData(), kGcmTagSize) != 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("SM4-GCM authentication failed");
        }
        return false;
    }

    plainText->resize(cipherText.size());
    std::array<quint8, 16> counter = initialCounter;
    for (qsizetype offset = 0; offset < cipherText.size(); offset += 16) {
        incrementCounter(&counter);
        std::array<quint8, 16> stream {};
        if (!cipher.encryptBlock(counter, &stream)) {
            return false;
        }
        const qsizetype bytes = qMin<qsizetype>(16, cipherText.size() - offset);
        for (qsizetype i = 0; i < bytes; ++i) {
            (*plainText)[offset + i] = static_cast<char>(static_cast<quint8>(cipherText.at(offset + i))
                                                        ^ stream[static_cast<size_t>(i)]);
        }
    }
    return true;
}

PkeyPtr createSm2PublicKey(const QByteArray &publicKey, QString *errorMessage)
{
    PkeyContextPtr context(EVP_PKEY_CTX_new_from_name(nullptr, "SM2", nullptr), EVP_PKEY_CTX_free);
    if (!context || EVP_PKEY_fromdata_init(context.get()) <= 0) {
        if (errorMessage) {
            *errorMessage = opensslError(QStringLiteral("Unable to initialize the SM2 public key loader"));
        }
        return PkeyPtr(nullptr, EVP_PKEY_free);
    }

    char groupName[] = "SM2";
    OSSL_PARAM parameters[] = {
        OSSL_PARAM_construct_utf8_string(OSSL_PKEY_PARAM_GROUP_NAME, groupName, 0),
        OSSL_PARAM_construct_octet_string(OSSL_PKEY_PARAM_PUB_KEY,
                                          const_cast<char *>(publicKey.constData()), publicKey.size()),
        OSSL_PARAM_construct_end()
    };
    EVP_PKEY *rawKey = nullptr;
    if (EVP_PKEY_fromdata(context.get(), &rawKey, EVP_PKEY_PUBLIC_KEY, parameters) <= 0) {
        if (errorMessage) {
            *errorMessage = opensslError(QStringLiteral("Unable to load the SM2 signing public key"));
        }
        return PkeyPtr(nullptr, EVP_PKEY_free);
    }
    return PkeyPtr(rawKey, EVP_PKEY_free);
}

bool verifySm2Signature(const QByteArray &publicKey, const QByteArray &sm2Id, const QByteArray &message,
                        const QByteArray &signature, QString *errorMessage)
{
    PkeyPtr key = createSm2PublicKey(publicKey, errorMessage);
    MdContextPtr digestContext(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    EVP_PKEY_CTX *keyContext = nullptr;
    if (!key || !digestContext
        || EVP_DigestVerifyInit(digestContext.get(), &keyContext, EVP_sm3(), nullptr, key.get()) <= 0
        || !keyContext
        || EVP_PKEY_CTX_set1_id(keyContext, sm2Id.constData(), sm2Id.size()) <= 0
        || EVP_DigestVerifyUpdate(digestContext.get(), message.constData(), message.size()) <= 0) {
        if (errorMessage && errorMessage->isEmpty()) {
            *errorMessage = opensslError(QStringLiteral("Unable to initialize SM2 signature verification"));
        }
        return false;
    }

    if (EVP_DigestVerifyFinal(digestContext.get(),
                              reinterpret_cast<const unsigned char *>(signature.constData()), signature.size()) != 1) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("SM2 response signature verification failed");
        }
        return false;
    }
    return true;
}

QString rejectedMessage(const QJsonObject &object)
{
    const QString code = object.value(QLatin1String("errorCode")).toString(QStringLiteral("UNKNOWN"));
    const qint64 retryAfter = object.value(QLatin1String("retryAfterMs")).toVariant().toLongLong();
    if (retryAfter > 0) {
        return QStringLiteral("SPA request rejected: %1; retry after %2 ms").arg(code).arg(retryAfter);
    }
    return QStringLiteral("SPA request rejected: %1").arg(code);
}

#ifdef SPA_DEBUG_LOGGING
QString indentedJsonForLog(QJsonObject object)
{
    if (object.contains(QLatin1String("spaTicket"))) {
        const qsizetype length = object.value(QLatin1String("spaTicket")).toString().size();
        object.insert(QLatin1String("spaTicket"), QStringLiteral("<redacted:%1-chars>").arg(length));
    }
    if (object.contains(QLatin1String("authProof"))) {
        object.insert(QLatin1String("authProof"), QStringLiteral("<redacted>"));
    }
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Indented)).trimmed();
}

QString requestFieldForLog(const QJsonObject &payload, const QString &name)
{
    if (!payload.contains(name) || payload.value(name).isNull() || payload.value(name).isUndefined()) {
        return QStringLiteral("<not-sent>");
    }

    const QJsonValue value = payload.value(name);
    if (value.isString()) {
        return value.toString();
    }
    if (value.isDouble()) {
        return QString::number(value.toVariant().toLongLong());
    }
    return QString::fromUtf8(QJsonDocument(QJsonObject { { name, value } })
                                     .toJson(QJsonDocument::Compact));
}

QString readableRequestFields(const QJsonObject &payload)
{
    return QStringLiteral("  requestId=%1\n"
                          "  clientTime=%2\n"
                          "  nonce=%3\n"
                          "  requestedService=%4\n"
                          "  installationId=%5\n"
                          "  platform=%6\n"
                          "  appVersion=%7\n"
                          "  authMode=%8\n"
                          "  credentialId=%9\n"
                          "  authProof=%10\n"
                          "  deviceId=%11")
            .arg(requestFieldForLog(payload, QStringLiteral("requestId")))
            .arg(requestFieldForLog(payload, QStringLiteral("clientTime")))
            .arg(requestFieldForLog(payload, QStringLiteral("nonce")))
            .arg(requestFieldForLog(payload, QStringLiteral("requestedService")))
            .arg(requestFieldForLog(payload, QStringLiteral("installationId")))
            .arg(requestFieldForLog(payload, QStringLiteral("platform")))
            .arg(requestFieldForLog(payload, QStringLiteral("appVersion")))
            .arg(requestFieldForLog(payload, QStringLiteral("authMode")))
            .arg(requestFieldForLog(payload, QStringLiteral("credentialId")))
            .arg(requestFieldForLog(payload, QStringLiteral("authProof")))
            .arg(requestFieldForLog(payload, QStringLiteral("deviceId")));
}

void logRequestPacket(quint8 version, const QByteArray &requestId, quint16 encryptionKeyId,
                      const QByteArray &clientPublicKey, const QByteArray &gcmNonce,
                      const QByteArray &plainText, const QByteArray &encryptedPayload,
                      const QByteArray &datagram, const QJsonObject &payload)
{
    qDebug().noquote()
            << QStringLiteral("[SPA][PACKET][TX]\n"
                              "  magic=ASPA version=%1 messageType=REQUEST\n"
                              "  encryptionKeyId=%2 requestId=%3\n"
                              "  clientEphemeralPublicKey=%4\n"
                              "  gcmNonce=%5 plaintextLength=%6 ciphertextLength=%7 packetLength=%8\n"
                              "  requestPayload=\n%9\n"
                              "  plaintextJson=\n%10\n"
                              // "  plaintextHex=%11\n"
                              // "  ciphertextAndGcmTagHex=%12\n"
                              "  udpDatagramHex=%13")
                       .arg(static_cast<unsigned int>(version))
                       .arg(encryptionKeyId)
                       .arg(QString::fromLatin1(base64Url(requestId)))
                       .arg(QString::fromLatin1(clientPublicKey.toHex()))
                       .arg(QString::fromLatin1(gcmNonce.toHex()))
                       .arg(plainText.size())
                       .arg(encryptedPayload.size())
                       .arg(datagram.size())
                       .arg(readableRequestFields(payload))
                       .arg(indentedJsonForLog(payload))
                       // .arg(QString::fromLatin1(plainText.toHex()))
                       // .arg(QString::fromLatin1(encryptedPayload.toHex()))
                       .arg(QString::fromLatin1(datagram.toHex()));
}

void logResponseHeader(const QByteArray &requestId, quint8 version,
                       quint16 encryptionKeyId, quint16 signingKeyId,
                       const QByteArray &gcmNonce, quint16 ciphertextLength,
                       quint16 signatureLength, const QByteArray &cipherText,
                       const QByteArray &signature, const QByteArray &datagram)
{
    qDebug().noquote()
            << QStringLiteral("[SPA][PACKET][RX]\n"
                              "  magic=ASPA version=%1 messageType=RESPONSE\n"
                              "  encryptionKeyId=%2 signingKeyId=%3 requestId=%4\n"
                              "  gcmNonce=%5 ciphertextLength=%6 signatureLength=%7 packetLength=%8\n"
                              "  ciphertextAndGcmTagHex=%9\n"
                              "  sm2DerSignatureHex=%10\n"
                              "  udpDatagramHex=%11")
                       .arg(static_cast<unsigned int>(version))
                       .arg(encryptionKeyId)
                       .arg(signingKeyId)
                       .arg(QString::fromLatin1(base64Url(requestId)))
                       .arg(QString::fromLatin1(gcmNonce.toHex()))
                       .arg(ciphertextLength)
                       .arg(signatureLength)
                       .arg(datagram.size())
                       .arg(QString::fromLatin1(cipherText.toHex()))
                       .arg(QString::fromLatin1(signature.toHex()))
                       .arg(QString::fromLatin1(datagram.toHex()));
}

void logResponsePayload(const QByteArray &plainText, const QJsonObject &payload)
{
    qDebug().noquote() << QStringLiteral("[SPA][PACKET][RX]\n"
                                          "  plaintextHex=%1\n"
                                          "  plaintextJson=\n%2")
                                 .arg(QString::fromLatin1(plainText.toHex()))
                                 .arg(indentedJsonForLog(payload));
}
#endif

} // namespace

namespace spa {

GmSpaCodec::GmSpaCodec(ProtocolConfig protocolConfig, DeviceIdentity deviceIdentity, QString appVersion)
    : m_protocolConfig(std::move(protocolConfig)),
      m_deviceIdentity(std::move(deviceIdentity)),
      m_appVersion(std::move(appVersion))
{
}

GmSpaCodec::~GmSpaCodec()
{
    clearRequestState();
}

bool GmSpaCodec::selfTest(QString *errorMessage)
{
    const QByteArray key = QByteArray::fromHex("0123456789abcdeffedcba9876543210");
    const QByteArray knownPlainText = key;
    const QByteArray knownCipherText = QByteArray::fromHex("681edf34d206965e86b3e94f536e4246");
    Sm4BlockCipher blockCipher(key);
    std::array<quint8, 16> input {};
    std::array<quint8, 16> output {};
    std::memcpy(input.data(), knownPlainText.constData(), input.size());
    if (!blockCipher.encryptBlock(input, &output)
        || QByteArray(reinterpret_cast<const char *>(output.data()), output.size()) != knownCipherText) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("SM4 known-answer self-test failed");
        }
        return false;
    }

    const QByteArray nonce = QByteArray::fromHex("000102030405060708090a0b");
    const QByteArray aad = QByteArrayLiteral("ASPA-GCM-SELF-TEST");
    const QByteArray plainText = QByteArrayLiteral("SM4-GCM authenticated payload");
    QByteArray cipherText;
    QString cryptoError;
    if (!sm4GcmEncrypt(key, nonce, aad, plainText, &cipherText, &cryptoError)) {
        if (errorMessage) {
            *errorMessage = cryptoError;
        }
        return false;
    }
    QByteArray decrypted;
    if (!sm4GcmDecrypt(key, nonce, aad, cipherText, &decrypted, &cryptoError) || decrypted != plainText) {
        if (errorMessage) {
            *errorMessage = cryptoError.isEmpty() ? QStringLiteral("SM4-GCM round-trip self-test failed") : cryptoError;
        }
        return false;
    }
    cipherText[cipherText.size() - 1] = static_cast<char>(cipherText.back() ^ 0x01);
    if (sm4GcmDecrypt(key, nonce, aad, cipherText, &decrypted, nullptr)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("SM4-GCM tamper self-test failed");
        }
        return false;
    }
    return true;
}

EncodeResult GmSpaCodec::encodeRequest(const Request &request)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    clearRequestState();

    EncodeResult result;
    if (request.requestId.size() != kRequestIdSize || !m_deviceIdentity.isValid()
        || m_protocolConfig.encryptionKey.publicKey.size() != kPublicKeySize) {
        result.errorMessage = QStringLiteral("Incomplete SPA request identity or key configuration");
        return result;
    }

    QString signingKeyError;
    if (!createSm2PublicKey(m_protocolConfig.signingKey.publicKey, &signingKeyError)) {
        result.errorMessage = signingKeyError;
        return result;
    }

    QByteArray clientPublicKey;
    QByteArray sharedSecret;
    if (!createSharedSecret(m_protocolConfig.encryptionKey.publicKey, &clientPublicKey,
                            &sharedSecret, &result.errorMessage)) {
        return result;
    }

    QByteArray requestKey = deriveKey(QByteArrayLiteral("ASPA-REQUEST-KEY-v1"), sharedSecret, request.requestId);
    m_responseKey = deriveKey(QByteArrayLiteral("ASPA-RESPONSE-KEY-v1"), sharedSecret, request.requestId);
    sharedSecret.fill('\0');

    const QByteArray requestNonce = randomBytes(32);
    const QByteArray gcmNonce = randomBytes(kGcmNonceSize);
    if (requestKey.size() != kSm4KeySize || m_responseKey.size() != kSm4KeySize
        || requestNonce.size() != 32 || gcmNonce.size() != kGcmNonceSize) {
        requestKey.fill('\0');
        clearRequestState();
        result.errorMessage = opensslError(QStringLiteral("Unable to generate SPA request key material"));
        return result;
    }

    QJsonObject payload {
        { QStringLiteral("requestId"), QString::fromLatin1(base64Url(request.requestId)) },
        { QStringLiteral("clientTime"), request.createdAtUtc.toMSecsSinceEpoch() },
        { QStringLiteral("nonce"), QString::fromLatin1(base64Url(requestNonce)) },
        { QStringLiteral("requestedService"), QStringLiteral("login") },
        { QStringLiteral("installationId"), m_deviceIdentity.installationId },
        { QStringLiteral("platform"), m_deviceIdentity.platform },
        { QStringLiteral("appVersion"), m_appVersion },
        { QStringLiteral("authMode"), QStringLiteral("DISCOVERY_ONLY") },
        { QStringLiteral("deviceId"), m_deviceIdentity.deviceId }
    };
    const QByteArray plainText = QJsonDocument(payload).toJson(QJsonDocument::Compact);

    const qsizetype cipherTextLength = plainText.size() + kGcmTagSize;
    if (cipherTextLength > std::numeric_limits<quint16>::max()) {
        requestKey.fill('\0');
        clearRequestState();
        result.errorMessage = QStringLiteral("SPA request is too large");
        return result;
    }

    QByteArray header;
    header.reserve(4 + 1 + 1 + 2 + kRequestIdSize + kPublicKeySize + kGcmNonceSize + 2);
    header.append(kMagic, 4);
    header.append(static_cast<char>(m_protocolConfig.version));
    header.append(static_cast<char>(kRequestType));
    appendUint16(header, m_protocolConfig.encryptionKey.keyId);
    header.append(request.requestId);
    header.append(clientPublicKey);
    header.append(gcmNonce);
    appendUint16(header, static_cast<quint16>(cipherTextLength));

    QByteArray encryptedPayload;
    if (!sm4GcmEncrypt(requestKey, gcmNonce, header, plainText, &encryptedPayload, &result.errorMessage)) {
        requestKey.fill('\0');
        clearRequestState();
        return result;
    }
    requestKey.fill('\0');

    result.datagram = header + encryptedPayload;
    if (result.datagram.size() > kMaximumDatagramSize) {
        result.datagram.clear();
        clearRequestState();
        result.errorMessage = QStringLiteral("SPA UDP datagram exceeds the protocol limit");
        return result;
    }
#ifdef SPA_DEBUG_LOGGING
    logRequestPacket(m_protocolConfig.version, request.requestId,
                     m_protocolConfig.encryptionKey.keyId,
                     clientPublicKey, gcmNonce, plainText, encryptedPayload,
                     result.datagram, payload);
#endif
    m_activeRequestId = request.requestId;
    return result;
}

DecodeResult GmSpaCodec::decodeResponse(const QByteArray &datagram, const Request &request)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (datagram.size() < kResponsePrefixSize + 2 || request.requestId != m_activeRequestId
        || m_responseKey.size() != kSm4KeySize) {
        return DecodeResult::ignored();
    }
    if (datagram.left(4) != QByteArray(kMagic, 4)
        || static_cast<quint8>(datagram.at(4)) != m_protocolConfig.version
        || static_cast<quint8>(datagram.at(5)) != kResponseType) {
        return DecodeResult::ignored();
    }

    quint16 encryptionKeyId = 0;
    quint16 signingKeyId = 0;
    quint16 cipherTextLength = 0;
    if (!readUint16(datagram, kResponseEncryptionKeyIdOffset, &encryptionKeyId)
        || encryptionKeyId != m_protocolConfig.encryptionKey.keyId
        || !readUint16(datagram, kResponseSigningKeyIdOffset, &signingKeyId)
        || signingKeyId != m_protocolConfig.signingKey.keyId
        || datagram.mid(kResponseRequestIdOffset, kRequestIdSize) != request.requestId
        || !readUint16(datagram, kResponseCiphertextLengthOffset, &cipherTextLength)) {
        return DecodeResult::ignored();
    }

    const qsizetype cipherTextOffset = kResponsePrefixSize;
    const qsizetype signatureLengthOffset = cipherTextOffset + cipherTextLength;
    quint16 signatureLength = 0;
    if (cipherTextLength < kGcmTagSize
        || !readUint16(datagram, signatureLengthOffset, &signatureLength)
        || signatureLength == 0
        || signatureLengthOffset + 2 + signatureLength != datagram.size()) {
        return DecodeResult::rejected(QStringLiteral("Malformed SPA response lengths"));
    }

    const QByteArray prefixHeader = datagram.left(kResponsePrefixSize);
    const QByteArray cipherText = datagram.mid(cipherTextOffset, cipherTextLength);
    const QByteArray signature = datagram.mid(signatureLengthOffset + 2, signatureLength);
#ifdef SPA_DEBUG_LOGGING
    logResponseHeader(request.requestId, static_cast<quint8>(datagram.at(4)),
                      encryptionKeyId, signingKeyId,
                      datagram.mid(kResponseGcmNonceOffset, kGcmNonceSize),
                      cipherTextLength, signatureLength, cipherText, signature, datagram);
#endif
    QString errorMessage;
    if (!verifySm2Signature(m_protocolConfig.signingKey.publicKey, m_protocolConfig.sm2Id,
                            prefixHeader + cipherText, signature, &errorMessage)) {
        clearRequestState();
        return DecodeResult::rejected(errorMessage);
    }

    const QByteArray gcmNonce = datagram.mid(kResponseGcmNonceOffset, kGcmNonceSize);
    QByteArray plainText;
    if (!sm4GcmDecrypt(m_responseKey, gcmNonce, prefixHeader, cipherText, &plainText, &errorMessage)) {
        clearRequestState();
        return DecodeResult::rejected(errorMessage);
    }
    clearRequestState();

    QJsonParseError jsonError;
    const QJsonDocument document = QJsonDocument::fromJson(plainText, &jsonError);
    if (jsonError.error != QJsonParseError::NoError || !document.isObject()) {
        plainText.fill('\0');
        return DecodeResult::rejected(QStringLiteral("Invalid SPA response JSON"));
    }

    const QJsonObject object = document.object();
#ifdef SPA_DEBUG_LOGGING
    logResponsePayload(plainText, object);
#endif
    plainText.fill('\0');
    if (object.value(QLatin1String("requestId")).toString().toLatin1() != base64Url(request.requestId)) {
        return DecodeResult::rejected(QStringLiteral("SPA response requestId mismatch"));
    }
    const QString status = object.value(QLatin1String("status")).toString();
    if (status == QLatin1String("REJECTED")) {
        return DecodeResult::rejected(rejectedMessage(object));
    }
    if (status != QLatin1String("OK")) {
        return DecodeResult::rejected(QStringLiteral("Unknown SPA response status"));
    }

    const qint64 issuedAt = object.value(QLatin1String("issuedAt")).toVariant().toLongLong();
    const qint64 expiresAt = object.value(QLatin1String("expiresAt")).toVariant().toLongLong();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (issuedAt <= 0 || expiresAt <= issuedAt
        || qAbs(now - issuedAt) > m_protocolConfig.maximumClockSkewMsecs) {
        return DecodeResult::rejected(QStringLiteral("SPA response timestamp is invalid"));
    }

    const int port = object.value(QLatin1String("loginPort")).toInt();
    if (port <= 0 || port > 65535) {
        return DecodeResult::rejected(QStringLiteral("Invalid login port in SPA response"));
    }

    LoginEndpoint endpoint;
    endpoint.scheme = object.value(QLatin1String("loginScheme")).toString();
    endpoint.host = object.value(QLatin1String("loginHost")).toString();
    endpoint.port = static_cast<quint16>(port);
    endpoint.ticket = object.value(QLatin1String("spaTicket")).toString().toUtf8();
    endpoint.expiresAtUtc = QDateTime::fromMSecsSinceEpoch(expiresAt, Qt::UTC);
    return DecodeResult::accepted(std::move(endpoint));
}

void GmSpaCodec::clearRequestState()
{
    m_activeRequestId.fill('\0');
    m_activeRequestId.clear();
    m_responseKey.fill('\0');
    m_responseKey.clear();
}

} // namespace spa
