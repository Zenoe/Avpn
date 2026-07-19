#ifndef SERVERCONFIGUTILS_H
#define SERVERCONFIGUTILS_H

#include <QJsonObject>

namespace serverConfigUtils
{

enum ConfigType {
    SelfHostedAdmin = 0,
    SelfHostedUser,
    Native,
    Invalid
};

ConfigType configTypeFromJson(const QJsonObject &serverConfigObject);

} // namespace serverConfigUtils

#endif // SERVERCONFIGUTILS_H
