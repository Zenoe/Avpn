#include "connectionController.h"

#include "core/configurators/configuratorBase.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/utilities.h"
#include "version.h"

using namespace caelispect;

ConnectionController::ConnectionController(SecureServersRepository *serversRepository,
                                           SecureAppSettingsRepository *appSettingsRepository,
                                           VpnConnection *vpnConnection, QObject *parent)
    : QObject(parent), m_serversRepository(serversRepository), m_appSettingsRepository(appSettingsRepository), m_vpnConnection(vpnConnection)
{
    connect(m_vpnConnection, &VpnConnection::connectionStateChanged,
            this, &ConnectionController::connectionStateChanged);
    connect(this, &ConnectionController::openConnectionRequested, m_vpnConnection, &VpnConnection::connectToVpn, Qt::QueuedConnection);
    connect(this, &ConnectionController::closeConnectionRequested, m_vpnConnection, &VpnConnection::disconnectFromVpn, Qt::QueuedConnection);
    connect(this, &ConnectionController::setConnectionStateRequested, m_vpnConnection, &VpnConnection::setConnectionState, Qt::QueuedConnection);
    connect(this, &ConnectionController::killSwitchModeChangedRequested, m_vpnConnection, &VpnConnection::onKillSwitchModeChanged, Qt::QueuedConnection);
#ifdef Q_OS_ANDROID
    connect(this, &ConnectionController::restoreConnectionRequested, m_vpnConnection, &VpnConnection::restoreConnection, Qt::QueuedConnection);
#endif
}

ErrorCode ConnectionController::isConnectionSupported(const QString &serverId) const
{
    if (serverId.isEmpty() || !m_serversRepository->wireGuardProfile(serverId)) return ErrorCode::InternalError;
    return isServiceReady() ? ErrorCode::NoError : ErrorCode::CaelispectServiceNotRunning;
}

ErrorCode ConnectionController::prepareConnection(const QString &serverId, QJsonObject &vpnConfiguration)
{
    const auto profile = m_serversRepository->wireGuardProfile(serverId);
    if (!profile) return ErrorCode::InternalError;
    const auto dns = profile->dnsPair(m_appSettingsRepository->primaryDns(), m_appSettingsRepository->secondaryDns());
    WireGuardProtocolConfig protocol;
    protocol.serverConfig.port = QString::number(profile->config.port);
    protocol.serverConfig.transportProto = QStringLiteral("udp");
    protocol.clientConfig = profile->config;
    ConnectionSettings settings = { { dns.first, dns.second }, false,
                                    { m_appSettingsRepository->isSitesSplitTunnelingEnabled(), m_appSettingsRepository->routeMode() } };
    const ProtocolConfig processed = ConfiguratorBase::create(Proto::WireGuard)->processConfigWithLocalSettings(settings, protocol);
    QJsonObject config = processed.getClientConfigJson();
    if (config.value(configKey::mtu).toString().isEmpty()) config[configKey::mtu] = protocols::wireguard::defaultMtu;
    vpnConfiguration[ProtocolUtils::key_proto_config_data(Proto::WireGuard)] = config;
    vpnConfiguration[configKey::vpnProto] = ProtocolUtils::protoToString(Proto::WireGuard);
    vpnConfiguration[configKey::dns1] = dns.first;
    vpnConfiguration[configKey::dns2] = dns.second;
    vpnConfiguration[configKey::hostName] = profile->hostName;
    vpnConfiguration[configKey::description] = profile->displayName();
    return ErrorCode::NoError;
}

ErrorCode ConnectionController::openConnection(const QString &serverId)
{
    const ErrorCode supported = isConnectionSupported(serverId);
    if (supported != ErrorCode::NoError) return supported;
    QJsonObject config;
    const ErrorCode result = prepareConnection(serverId, config);
    if (result == ErrorCode::NoError) emit openConnectionRequested(serverId, config);
    return result;
}

void ConnectionController::closeConnection() { emit closeConnectionRequested(); }
#ifdef Q_OS_ANDROID
void ConnectionController::restoreConnection() { emit restoreConnectionRequested(); }
#endif
void ConnectionController::onKillSwitchModeChanged(bool enabled) { emit killSwitchModeChangedRequested(enabled); }
ErrorCode ConnectionController::lastConnectionError() const { return m_vpnConnection->lastError(); }
bool ConnectionController::isConnected() const { return m_vpnConnection->connectionState() == Vpn::ConnectionState::Connected; }
void ConnectionController::setConnectionState(Vpn::ConnectionState state) { emit setConnectionStateRequested(state); }
bool ConnectionController::isServiceReady() const
{
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
    return Utils::processIsRunning(Utils::executable(SERVICE_NAME, false), true);
#else
    return true;
#endif
}
