#include "dnsProtocolConfig.h"

namespace caelispect
{

QJsonObject DnsProtocolConfig::toJson() const
{
    return QJsonObject();
}

DnsProtocolConfig DnsProtocolConfig::fromJson(const QJsonObject& json)
{
    Q_UNUSED(json);
    return DnsProtocolConfig();
}

} // namespace caelispect

