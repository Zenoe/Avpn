#ifndef SERVERDESCRIPTION_H
#define SERVERDESCRIPTION_H

#include <QString>

#include "core/utils/containerEnum.h"
#include "core/utils/commonStructs.h"
#include "core/models/selfhosted/selfHostedAdminServerConfig.h"
#include "core/models/selfhosted/selfHostedUserServerConfig.h"
#include "core/models/selfhosted/nativeServerConfig.h"

namespace caelispect
{

struct ServerDescription
{
    QString serverId;

    QString serverName;
    QString baseDescription;
    QString hostName;

    ServerCredentials selfHostedSshCredentials;
    bool hasWriteAccess = false;

    bool primaryDnsIsCaelispect = false;
    DockerContainer defaultContainer = DockerContainer::None;
    bool hasInstalledVpnContainers = false;

    QString collapsedServerDescription;
    QString expandedServerDescription;
};

ServerDescription buildServerDescription(const SelfHostedAdminServerConfig &server, bool isCaelispectDnsEnabled);
ServerDescription buildServerDescription(const SelfHostedUserServerConfig &server, bool isCaelispectDnsEnabled);
ServerDescription buildServerDescription(const NativeServerConfig &server, bool isCaelispectDnsEnabled);

} // namespace caelispect

#endif
