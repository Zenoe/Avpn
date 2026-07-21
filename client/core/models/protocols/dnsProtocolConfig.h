#ifndef DNSPROTOCOLCONFIG_H
#define DNSPROTOCOLCONFIG_H

#include <QJsonObject>

namespace caelispect
{

struct DnsProtocolConfig {
    QJsonObject toJson() const;
    static DnsProtocolConfig fromJson(const QJsonObject& json);
};

} // namespace caelispect

#endif // DNSPROTOCOLCONFIG_H

