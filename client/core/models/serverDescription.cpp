#include "serverDescription.h"

#include <QMap>

#include "core/utils/serverConfigUtils.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/containers/containerUtils.h"
#include "core/protocols/protocolUtils.h"

using namespace caelispect;

namespace
{

bool computeHasInstalledVpnContainers(const QMap<DockerContainer, ContainerConfig> &containers)
{
    for (auto it = containers.begin(); it != containers.end(); ++it) {
        const DockerContainer container = it.key();
        if (ContainerUtils::containerService(container) == ServiceType::Vpn) {
            return true;
        }
    }
    return false;
}

template <typename T>
ServerDescription buildBaseDescription(const T &server)
{
    ServerDescription row;
    row.hostName = server.hostName;
    row.defaultContainer = server.defaultContainer;
    row.primaryDnsIsCaelispect = (server.dns1 == protocols::dns::caelispectDnsIp);
    row.hasInstalledVpnContainers = computeHasInstalledVpnContainers(server.containers);
    return row;
}

QString getBaseDescription(const QMap<DockerContainer, ContainerConfig> &containers,
                         bool isCaelispectDnsEnabled,
                         bool hasWriteAccess,
                         bool primaryDnsIsCaelispect)
{
    QString description;
    if (hasWriteAccess) {
        const bool isDnsInstalled = containers.contains(DockerContainer::Dns);
        if (isCaelispectDnsEnabled && isDnsInstalled) {
            description += QStringLiteral("Caelispect DNS | ");
        }
    } else if (primaryDnsIsCaelispect) {
        description += QStringLiteral("Caelispect DNS | ");
    }
    return description;
}

QString getProtocolName(DockerContainer defaultContainer, const QMap<DockerContainer, ContainerConfig> &containers)
{
    QString containerName = ContainerUtils::containerHumanNames().value(defaultContainer);
    return containerName + QStringLiteral(" | ");
}

} // namespace

namespace caelispect
{

ServerDescription buildServerDescription(const SelfHostedAdminServerConfig &server, bool isCaelispectDnsEnabled)
{
    ServerDescription row = buildBaseDescription(server);
    row.selfHostedSshCredentials.hostName = server.hostName;
    row.selfHostedSshCredentials.userName = server.userName;
    row.selfHostedSshCredentials.secretData = server.password;
    row.selfHostedSshCredentials.port = server.port > 0 ? server.port : 22;

    row.hasWriteAccess = !row.selfHostedSshCredentials.userName.isEmpty()
                         && !row.selfHostedSshCredentials.secretData.isEmpty();

    row.serverName = server.displayName;
    row.baseDescription = getBaseDescription(server.containers, isCaelispectDnsEnabled, row.hasWriteAccess, row.primaryDnsIsCaelispect);

    const QString protocolName = getProtocolName(server.defaultContainer, server.containers);
    row.expandedServerDescription = row.baseDescription + row.hostName;
    row.collapsedServerDescription = row.baseDescription + protocolName + row.hostName;
    return row;
}

ServerDescription buildServerDescription(const SelfHostedUserServerConfig &server, bool isCaelispectDnsEnabled)
{
    ServerDescription row = buildBaseDescription(server);
    row.selfHostedSshCredentials.hostName = server.hostName;
    row.selfHostedSshCredentials.port = 22;
    row.hasWriteAccess = false;

    row.serverName = server.displayName;
    row.baseDescription = getBaseDescription(server.containers, isCaelispectDnsEnabled, row.hasWriteAccess, row.primaryDnsIsCaelispect);

    const QString protocolName = getProtocolName(server.defaultContainer, server.containers);
    row.expandedServerDescription = row.baseDescription + row.hostName;
    row.collapsedServerDescription = row.baseDescription + protocolName + row.hostName;
    return row;
}

ServerDescription buildServerDescription(const NativeServerConfig &server, bool isCaelispectDnsEnabled)
{
    ServerDescription row = buildBaseDescription(server);
    row.hasWriteAccess = false;

    row.serverName = server.displayName;
    row.baseDescription = getBaseDescription(server.containers, isCaelispectDnsEnabled, row.hasWriteAccess, row.primaryDnsIsCaelispect);

    const QString protocolName = getProtocolName(server.defaultContainer, server.containers);
    row.expandedServerDescription = row.baseDescription + row.hostName;
    row.collapsedServerDescription = row.baseDescription + protocolName + row.hostName;
    return row;
}

} // namespace caelispect
