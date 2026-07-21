#ifndef SPA_CONFIG_H
#define SPA_CONFIG_H

#include <QByteArray>
#include <QString>

#include "spa/spaTypes.h"

namespace spa {

struct KeyConfig
{
    quint16 keyId = 0;
    QString name;
    QByteArray publicKey;
};

struct ProtocolConfig
{
    quint8 version = 1;
    QString cipherSuite;
    QByteArray sm2Id;
    int maximumClockSkewMsecs = 60000;
    KeyConfig encryptionKey;
    KeyConfig signingKey;
};

struct DefaultConfig
{
    quint16 gatewayPort = 0;
    ProtocolConfig protocol;
    ClientConfig client;
};

class ConfigLoader final
{
public:
    static DefaultConfig loadDefault(QString *errorMessage = nullptr);
};

} // namespace spa

#endif // SPA_CONFIG_H
