#ifndef WIREGUARD_CONFIGURATOR_H
#define WIREGUARD_CONFIGURATOR_H

#include <QHostAddress>
#include <QObject>
#include <QProcessEnvironment>

#include "configuratorBase.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"

class WireguardConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    WireguardConfigurator(bool isAwg, QObject *parent = nullptr);

    struct ConnectionData
    {
        QString clientPrivKey; // client private key
        QString clientPubKey;  // client public key
        QString clientIP;      // internal client IP address
        QString serverPubKey;  // tls-auth key
        QString pskKey;        // preshared key
        QString host;          // host ip
        QString port;
    };

    amnezia::ProtocolConfig processConfigWithLocalSettings(const amnezia::ConnectionSettings &settings,
                                                           amnezia::ProtocolConfig protocolConfig) override;
    amnezia::ProtocolConfig processConfigWithExportSettings(const amnezia::ExportSettings &settings,
                                                            amnezia::ProtocolConfig protocolConfig) override;

    static ConnectionData genClientKeys();

private:
    bool m_isAwg;
};

#endif // WIREGUARD_CONFIGURATOR_H
