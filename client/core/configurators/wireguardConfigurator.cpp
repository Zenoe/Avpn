#include "wireguardConfigurator.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include "core/utils/utilities.h"

using namespace caelispect;

WireguardConfigurator::WireguardConfigurator(bool isAwg, QObject *parent)
    : ConfiguratorBase(parent), m_isAwg(isAwg)
{
}

WireguardConfigurator::ConnectionData WireguardConfigurator::genClientKeys()
{
    constexpr size_t keyLength = 32;
    ConnectionData connectionData;

    unsigned char buffer[keyLength];
    if (RAND_priv_bytes(buffer, keyLength) <= 0) {
        return connectionData;
    }

    EVP_PKEY *key = EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, nullptr, buffer, keyLength);
    q_check_ptr(key);

    size_t keySize = keyLength;
    unsigned char privateKey[keyLength];
    EVP_PKEY_get_raw_private_key(key, privateKey, &keySize);
    connectionData.clientPrivKey = QByteArray::fromRawData(reinterpret_cast<char *>(privateKey), keySize).toBase64();

    unsigned char publicKey[keyLength];
    EVP_PKEY_get_raw_public_key(key, publicKey, &keySize);
    connectionData.clientPubKey = QByteArray::fromRawData(reinterpret_cast<char *>(publicKey), keySize).toBase64();
    EVP_PKEY_free(key);

    return connectionData;
}

ProtocolConfig WireguardConfigurator::processConfigWithLocalSettings(const ConnectionSettings &settings,
                                                                     ProtocolConfig protocolConfig)
{
    return ConfiguratorBase::processConfigWithLocalSettings(settings, protocolConfig);
}

ProtocolConfig WireguardConfigurator::processConfigWithExportSettings(const ExportSettings &settings,
                                                                      ProtocolConfig protocolConfig)
{
    return ConfiguratorBase::processConfigWithExportSettings(settings, protocolConfig);
}
