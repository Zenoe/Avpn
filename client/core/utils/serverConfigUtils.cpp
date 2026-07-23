#include "serverConfigUtils.h"

#include "core/models/wireguardProfile.h"

namespace serverConfigUtils
{

ConfigType configTypeFromJson(const QJsonObject &serverConfigObject)
{
    return caelispect::WireGuardProfile::isValidJson(serverConfigObject)
            ? ConfigType::WireGuardProfile : ConfigType::Invalid;
}

} // namespace serverConfigUtils
