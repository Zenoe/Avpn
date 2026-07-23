#include "protocolConfig.h"

namespace caelispect
{
Proto ProtocolConfig::type() const { return Proto::WireGuard; }
QString ProtocolConfig::port() const { return std::get<WireGuardProtocolConfig>(data).serverConfig.port; }
QString ProtocolConfig::transportProto() const { return std::get<WireGuardProtocolConfig>(data).serverConfig.transportProto; }
bool ProtocolConfig::hasClientConfig() const { return std::get<WireGuardProtocolConfig>(data).hasClientConfig(); }
QString ProtocolConfig::clientId() const
{
    const auto &config = std::get<WireGuardProtocolConfig>(data);
    return config.clientConfig ? config.clientConfig->clientId : QString();
}
QJsonObject ProtocolConfig::getClientConfigJson() const
{
    const auto &config = std::get<WireGuardProtocolConfig>(data);
    return config.clientConfig ? config.clientConfig->toJson() : QJsonObject();
}
void ProtocolConfig::setClientConfigJson(const QJsonObject &json) { std::get<WireGuardProtocolConfig>(data).setClientConfig(WireGuardClientConfig::fromJson(json)); }
QString ProtocolConfig::nativeConfig() const
{
    const auto &config = std::get<WireGuardProtocolConfig>(data);
    return config.clientConfig ? config.clientConfig->nativeConfig : QString();
}
void ProtocolConfig::setNativeConfig(const QString &config)
{
    auto &wg = std::get<WireGuardProtocolConfig>(data);
    if (wg.clientConfig) wg.clientConfig->nativeConfig = config;
}
QJsonObject ProtocolConfig::toJson() const { return std::get<WireGuardProtocolConfig>(data).toJson(); }
ProtocolConfig ProtocolConfig::fromJson(const QJsonObject &json, Proto type)
{
    return type == Proto::WireGuard ? ProtocolConfig { WireGuardProtocolConfig::fromJson(json) } : ProtocolConfig {};
}
} // namespace caelispect
