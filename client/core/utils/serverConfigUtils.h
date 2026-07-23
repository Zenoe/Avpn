#ifndef SERVERCONFIGUTILS_H
#define SERVERCONFIGUTILS_H

#include <QJsonObject>

namespace serverConfigUtils
{

enum ConfigType { WireGuardProfile = 0, Invalid };

ConfigType configTypeFromJson(const QJsonObject &serverConfigObject);

} // namespace serverConfigUtils

#endif // SERVERCONFIGUTILS_H
