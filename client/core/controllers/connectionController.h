#ifndef CONNECTIONCONTROLLER_H
#define CONNECTIONCONTROLLER_H

#include <QObject>
#include <QJsonObject>

#include "core/repositories/secureAppSettingsRepository.h"
#include "core/repositories/secureServersRepository.h"
#include "vpnConnection.h"

class ConnectionController : public QObject
{
    Q_OBJECT
public:
    explicit ConnectionController(SecureServersRepository *serversRepository,
                                  SecureAppSettingsRepository *appSettingsRepository,
                                  VpnConnection *vpnConnection, QObject *parent = nullptr);
    caelispect::ErrorCode prepareConnection(const QString &serverId, QJsonObject &vpnConfiguration);
    caelispect::ErrorCode isConnectionSupported(const QString &serverId) const;
    caelispect::ErrorCode openConnection(const QString &serverId);
    void closeConnection();
#ifdef Q_OS_ANDROID
    void restoreConnection();
#endif
    void onKillSwitchModeChanged(bool enabled);
    caelispect::ErrorCode lastConnectionError() const;
    bool isConnected() const;
    void setConnectionState(Vpn::ConnectionState state);
    bool isServiceReady() const;

signals:
    void connectionStateChanged(Vpn::ConnectionState state);
    void openConnectionRequested(const QString &serverId, const QJsonObject &vpnConfiguration);
    void closeConnectionRequested();
    void setConnectionStateRequested(Vpn::ConnectionState state);
    void killSwitchModeChangedRequested(bool enabled);
#ifdef Q_OS_ANDROID
    void restoreConnectionRequested();
#endif

private:
    SecureServersRepository *m_serversRepository;
    SecureAppSettingsRepository *m_appSettingsRepository;
    VpnConnection *m_vpnConnection;
};

#endif
