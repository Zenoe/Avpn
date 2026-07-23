#include "wireguardProfile.h"

#include "core/utils/constants/configKeys.h"

namespace caelispect
{

QString WireGuardProfile::displayName() const
{
    return description.isEmpty() ? hostName : description;
}

QPair<QString, QString> WireGuardProfile::dnsPair(const QString &fallbackPrimary, const QString &fallbackSecondary) const
{
    const QString primary = dns1.isEmpty() ? fallbackPrimary : dns1;
    const QString secondary = dns2.isEmpty() ? fallbackSecondary : dns2;
    return { primary, secondary };
}

QJsonObject WireGuardProfile::toJson() const
{
    QJsonObject json;
    json[QStringLiteral("profileType")] = QStringLiteral("wireguard");
    json[configKey::description] = description;
    json[configKey::hostName] = hostName;
    if (!dns1.isEmpty()) json[configKey::dns1] = dns1;
    if (!dns2.isEmpty()) json[configKey::dns2] = dns2;
    json[configKey::wireguard] = config.toJson();
    return json;
}

WireGuardProfile WireGuardProfile::fromJson(const QJsonObject &json)
{
    WireGuardProfile profile;
    profile.description = json.value(configKey::description).toString();
    profile.hostName = json.value(configKey::hostName).toString();
    profile.dns1 = json.value(configKey::dns1).toString();
    profile.dns2 = json.value(configKey::dns2).toString();
    profile.config = WireGuardClientConfig::fromJson(json.value(configKey::wireguard).toObject());
    return profile;
}

bool WireGuardProfile::isValidJson(const QJsonObject &json)
{
    if (json.value(QStringLiteral("profileType")).toString() != QStringLiteral("wireguard")) return false;
    const QJsonObject config = json.value(configKey::wireguard).toObject();
    return !json.value(configKey::hostName).toString().isEmpty()
           && !config.value(configKey::clientPrivKey).toString().isEmpty()
           && !config.value(configKey::clientIp).toString().isEmpty()
           && !config.value(configKey::serverPubKey).toString().isEmpty();
}

} // namespace caelispect
