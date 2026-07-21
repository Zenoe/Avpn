#include "serverConfigUtils.h"

#include <QJsonArray>
#include <QJsonValue>

#include "core/models/selfhosted/selfHostedAdminServerConfig.h"
#include "core/utils/constants/configKeys.h"

namespace
{

bool hasThirdPartyConfig(const QJsonObject &json)
{
    const QJsonArray containersArray = json.value(caelispect::configKey::containers).toArray();
    for (const QJsonValue &val : containersArray) {
        const QJsonObject containerObj = val.toObject();
        for (auto it = containerObj.begin(); it != containerObj.end(); ++it) {
            if (it.key() == caelispect::configKey::container) {
                continue;
            }
            const QJsonObject protocolObj = it.value().toObject();
            if (protocolObj.contains(caelispect::configKey::isThirdPartyConfig)
                && protocolObj.value(caelispect::configKey::isThirdPartyConfig).toBool()) {
                return true;
            }
        }
    }
    return false;
}

bool hasAwgContainer(const QJsonObject &json)
{
    const QJsonArray containersArray = json.value(caelispect::configKey::containers).toArray();
    for (const QJsonValue &value : containersArray) {
        const QString container = value.toObject().value(caelispect::configKey::container).toString();
        if (container == QStringLiteral("caelispect-awg") || container == QStringLiteral("caelispect-awg2")) {
            return true;
        }
    }
    return false;
}

} // namespace

namespace serverConfigUtils
{

ConfigType configTypeFromJson(const QJsonObject &serverConfigObject)
{
    if (hasAwgContainer(serverConfigObject)
        || serverConfigObject.value(caelispect::configKey::configVersion).toInt() != 0
        || serverConfigObject.contains(QStringLiteral("api_key"))
        || serverConfigObject.contains(QStringLiteral("auth_data"))) {
        return ConfigType::Invalid;
    }

    if (hasThirdPartyConfig(serverConfigObject)) {
        return ConfigType::Native;
    }

    const caelispect::SelfHostedAdminServerConfig adminProbe =
            caelispect::SelfHostedAdminServerConfig::fromJson(serverConfigObject);
    return adminProbe.hasCredentials() ? ConfigType::SelfHostedAdmin : ConfigType::SelfHostedUser;
}

} // namespace serverConfigUtils
