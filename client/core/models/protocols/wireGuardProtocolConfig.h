#ifndef WIREGUARDPROTOCOLCONFIG_H
#define WIREGUARDPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <optional>

namespace caelispect
{

struct WireGuardServerConfig {
    QString port;
    QString transportProto;
    QString subnetAddress;
    QString subnetMask;
    QString subnetCidr;
    
    QJsonObject toJson() const;
    static WireGuardServerConfig fromJson(const QJsonObject& json);
    
};

struct WireGuardClientConfig {
    QString nativeConfig;
    QString hostName;
    int port;
    QString clientIp;
    QString clientPrivateKey;
    QString clientPublicKey;
    QString serverPublicKey;
    QString presharedKey;
    QString clientId;
    QStringList allowedIps;
    QString persistentKeepAlive;
    QString mtu;
    
    QJsonObject toJson() const;
    static WireGuardClientConfig fromJson(const QJsonObject& json);
};

struct WireGuardProtocolConfig {
    WireGuardServerConfig serverConfig;
    std::optional<WireGuardClientConfig> clientConfig;
    
    QJsonObject toJson() const;
    static WireGuardProtocolConfig fromJson(const QJsonObject& json);
    
    bool hasClientConfig() const;
    void setClientConfig(const WireGuardClientConfig& config);
};

} // namespace caelispect

#endif // WIREGUARDPROTOCOLCONFIG_H

