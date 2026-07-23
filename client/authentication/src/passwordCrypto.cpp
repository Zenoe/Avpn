#include "passwordCrypto.h"

#include <QByteArray>

#include <memory>

#include <openssl/evp.h>
#include <openssl/crypto.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

namespace {

constexpr auto kLoginPublicKey =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAsgVBjBHXYL4W2/nCS0q1KYz3NoTmKPU+I5S+m7n6XWCiwT7x0O3DQWJBxamtTIUVoXlcTWYN5KLEZyYE+AOtpcadch06D+0obZbdSEsyO41eiJ+TfhWgtGp4P+rKDQTAHStl03gtWxDoGA+0L6tIOCZi4G3imx9A2jQvFD8lv96XLjqedsW7vMIE+fOSgfKvkBkVMWn7G9WVi6/xnbRUdSnPgTHgNadZv4420Z1oQbXLPkpNHZsMAYppqYY8RlSeVC8X/tm2HF+lplx+V99Kbz/UUmdZIq2zBMzVc5lGFVcRx8haIADmc4ng2a7bYPkSHwcVSgwqldXLZZ2bs15k/QIDAQAB";
constexpr auto kOaepLabel = "trust-login-password-v1";

using PkeyPtr = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
using PkeyContextPtr = std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>;

} // namespace

namespace authentication::crypto {

bool encryptPassword(const QString &password, QString *encryptedPassword, QString *errorMessage)
{
    if (!encryptedPassword || password.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("请输入密码");
        }
        return false;
    }

    const QByteArray publicKeyDer = QByteArray::fromBase64(kLoginPublicKey);
    const unsigned char *keyCursor = reinterpret_cast<const unsigned char *>(publicKeyDer.constData());
    PkeyPtr publicKey(d2i_PUBKEY(nullptr, &keyCursor, publicKeyDer.size()), EVP_PKEY_free);
    PkeyContextPtr context(publicKey ? EVP_PKEY_CTX_new(publicKey.get(), nullptr) : nullptr, EVP_PKEY_CTX_free);
    if (!context || EVP_PKEY_encrypt_init(context.get()) <= 0
        || EVP_PKEY_CTX_set_rsa_padding(context.get(), RSA_PKCS1_OAEP_PADDING) <= 0
        || EVP_PKEY_CTX_set_rsa_oaep_md(context.get(), EVP_sha256()) <= 0
        || EVP_PKEY_CTX_set_rsa_mgf1_md(context.get(), EVP_sha256()) <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法初始化登录密码加密");
        }
        return false;
    }

    unsigned char *label = static_cast<unsigned char *>(OPENSSL_memdup(kOaepLabel, sizeof(kOaepLabel) - 1));
    if (!label || EVP_PKEY_CTX_set0_rsa_oaep_label(context.get(), label, sizeof(kOaepLabel) - 1) <= 0) {
        OPENSSL_free(label);
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法设置登录密码加密标签");
        }
        return false;
    }

    QByteArray plainText = password.toUtf8();
    size_t encryptedSize = 0;
    const bool sized = EVP_PKEY_encrypt(context.get(), nullptr, &encryptedSize,
                                         reinterpret_cast<const unsigned char *>(plainText.constData()), plainText.size()) > 0;
    QByteArray encrypted(static_cast<qsizetype>(encryptedSize), Qt::Uninitialized);
    const bool encryptedOk = sized && EVP_PKEY_encrypt(context.get(),
        reinterpret_cast<unsigned char *>(encrypted.data()), &encryptedSize,
        reinterpret_cast<const unsigned char *>(plainText.constData()), plainText.size()) > 0;
    plainText.fill('\0');
    if (!encryptedOk || encryptedSize != 256) {
        encrypted.fill('\0');
        if (errorMessage) {
            *errorMessage = QStringLiteral("登录密码加密失败");
        }
        return false;
    }
    encrypted.resize(static_cast<qsizetype>(encryptedSize));
    *encryptedPassword = QString::fromLatin1(encrypted.toBase64());
    encrypted.fill('\0');
    return true;
}

} // namespace authentication::crypto
