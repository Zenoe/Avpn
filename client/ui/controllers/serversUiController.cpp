#include "serversUiController.h"

using namespace caelispect;

namespace { const ServerDescription emptyDescription {}; }

ServersUiController::ServersUiController(ServersController *serversController, SettingsController *settingsController,
                                         ServersModel *serversModel, WireGuardConfigModel *wireGuardConfigModel, QObject *parent)
    : QObject(parent), m_serversController(serversController), m_settingsController(settingsController),
      m_serversModel(serversModel), m_wireGuardConfigModel(wireGuardConfigModel) {}

void ServersUiController::removeServer(const QString &id) { if (!id.isEmpty()) { m_serversController->removeServer(id); updateModel(); } }
void ServersUiController::removeServerAtIndex(int index) { removeServer(getServerId(index)); }
void ServersUiController::editServerName(const QString &id, const QString &name)
{
    if (!m_serversController->renameServer(id, name)) emit errorOccurred(tr("Failed to update profile"));
    else updateModel();
}
void ServersUiController::setDefaultServer(const QString &id) { if (!id.isEmpty()) m_serversController->setDefaultServer(id); }
void ServersUiController::setDefaultServerAtIndex(int index) { setDefaultServer(getServerId(index)); }
void ServersUiController::openClientProtocolSettings(const QString &id)
{
    if (const auto profile = m_serversController->wireGuardProfile(id)) m_wireGuardConfigModel->updateModel(profile->config);
}
void ServersUiController::saveClientProtocolSettings(const QString &id)
{
    auto profile = m_serversController->wireGuardProfile(id);
    if (!profile) { emit errorOccurred(tr("Failed to update profile")); return; }
    profile->config = m_wireGuardConfigModel->getConfig();
    if (m_serversController->updateWireGuardProfile(id, *profile) == ErrorCode::NoError) emit finished(tr("Settings updated successfully"));
    else emit errorOccurred(tr("Failed to update profile"));
}
void ServersUiController::onDefaultServerChanged(const QString &id) { m_serversModel->setDefaultServerId(id); emit defaultServerIdChanged(id); }
bool ServersUiController::isDefaultServerCurrentlyProcessed() const { return getDefaultServerId() == m_processedServerId; }
void ServersUiController::setProcessedServerId(const QString &id)
{
    const QString next = getServerIndexById(id) >= 0 ? id : QString();
    if (m_processedServerId != next) { m_processedServerId = next; emit processedServerIdChanged(next); }
}
void ServersUiController::updateModel()
{
    m_descriptions = m_serversController->buildServerDescriptions(false);
    const QString id = m_serversController->getDefaultServerId();
    m_serversModel->updateModel(m_descriptions, id);
    if (getServerIndexById(m_processedServerId) < 0) setProcessedServerId({});
    emit defaultServerIdChanged(id);
}
QString ServersUiController::getDefaultServerId() const { return m_serversController->getDefaultServerId(); }
QString ServersUiController::getDefaultServerName() const { return description(getDefaultServerId()).serverName; }
QString ServersUiController::getDefaultServerDefaultContainerName() const { return QStringLiteral("WireGuard"); }
QString ServersUiController::getDefaultServerDescriptionCollapsed() const { return description(getDefaultServerId()).collapsedServerDescription; }
QString ServersUiController::getDefaultServerImagePathCollapsed() const { return {}; }
QString ServersUiController::getDefaultServerDescriptionExpanded() const { return description(getDefaultServerId()).expandedServerDescription; }
bool ServersUiController::isDefaultServerDefaultContainerHasSplitTunneling() const
{
    const auto profile = m_serversController->wireGuardProfile(getDefaultServerId());
    return profile && !profile->config.allowedIps.contains(QStringLiteral("0.0.0.0/0"));
}
QString ServersUiController::serverName(const QString &id) const { return description(id).serverName; }
QString ServersUiController::serverHostName(const QString &id) const { return description(id).hostName; }
QString ServersUiController::getProcessedServerId() const { return m_processedServerId; }
QString ServersUiController::getServerId(int index) const { return index >= 0 && index < m_descriptions.size() ? m_descriptions.at(index).serverId : QString(); }
int ServersUiController::getServerIndexById(const QString &id) const { for (int i = 0; i < m_descriptions.size(); ++i) if (m_descriptions.at(i).serverId == id) return i; return -1; }
int ServersUiController::getServersCount() const { return m_descriptions.size(); }
const ServerDescription &ServersUiController::description(const QString &id) const { for (const auto &item : m_descriptions) if (item.serverId == id) return item; return emptyDescription; }
