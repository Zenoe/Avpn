#include "serversController.h"

using namespace caelispect;

ServersController::ServersController(SecureServersRepository *serversRepository,
                                     SecureAppSettingsRepository *appSettingsRepository, QObject *parent)
    : QObject(parent), m_serversRepository(serversRepository), m_appSettingsRepository(appSettingsRepository)
{
    ensureDefaultServerValid();
}

void ServersController::ensureDefaultServerValid()
{
    if (m_serversRepository->serversCount() && m_serversRepository->defaultServerId().isEmpty())
        m_serversRepository->setDefaultServer(m_serversRepository->serverIdAt(0));
}

bool ServersController::renameServer(const QString &serverId, const QString &name)
{
    auto profile = wireGuardProfile(serverId);
    if (!profile) return false;
    profile->description = name.trimmed();
    m_serversRepository->editServer(serverId, profile->toJson(), serverConfigUtils::ConfigType::WireGuardProfile);
    return true;
}

void ServersController::removeServer(const QString &serverId) { m_serversRepository->removeServer(serverId); }
void ServersController::setDefaultServer(const QString &serverId) { m_serversRepository->setDefaultServer(serverId); }

QVector<ServerDescription> ServersController::buildServerDescriptions(bool) const
{
    QVector<ServerDescription> result;
    for (const QString &id : m_serversRepository->orderedServerIds()) {
        const auto profile = wireGuardProfile(id);
        if (!profile) continue;
        ServerDescription description = buildServerDescription(*profile);
        description.serverId = id;
        result.append(description);
    }
    return result;
}

int ServersController::getDefaultServerIndex() const { return m_serversRepository->defaultServerIndex(); }
QString ServersController::getDefaultServerId() const { return m_serversRepository->defaultServerId(); }
int ServersController::getServersCount() const { return m_serversRepository->serversCount(); }
QString ServersController::getServerId(int index) const { return m_serversRepository->serverIdAt(index); }
int ServersController::indexOfServerId(const QString &id) const { return m_serversRepository->indexOfServerId(id); }

QString ServersController::notificationDisplayName(const QString &serverId) const
{
    const auto profile = wireGuardProfile(serverId);
    return profile ? profile->displayName() : QString();
}

std::optional<WireGuardProfile> ServersController::wireGuardProfile(const QString &serverId) const
{
    return m_serversRepository->wireGuardProfile(serverId);
}

ErrorCode ServersController::updateWireGuardProfile(const QString &serverId, const WireGuardProfile &profile)
{
    if (!wireGuardProfile(serverId)) return ErrorCode::InternalError;
    m_serversRepository->editServer(serverId, profile.toJson(), serverConfigUtils::ConfigType::WireGuardProfile);
    return ErrorCode::NoError;
}
