#ifndef SERVERSCONTROLLER_H
#define SERVERSCONTROLLER_H

#include <QObject>
#include <QVector>
#include <optional>

#include "core/models/serverDescription.h"
#include "core/utils/errorCodes.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/repositories/secureServersRepository.h"

class ServersController : public QObject
{
    Q_OBJECT
public:
    explicit ServersController(SecureServersRepository *serversRepository,
                               SecureAppSettingsRepository *appSettingsRepository = nullptr, QObject *parent = nullptr);

    bool renameServer(const QString &serverId, const QString &name);
    void removeServer(const QString &serverId);
    void setDefaultServer(const QString &serverId);
    QVector<caelispect::ServerDescription> buildServerDescriptions(bool) const;
    int getDefaultServerIndex() const;
    QString getDefaultServerId() const;
    int getServersCount() const;
    QString getServerId(int serverIndex) const;
    int indexOfServerId(const QString &serverId) const;
    QString notificationDisplayName(const QString &serverId) const;
    std::optional<caelispect::WireGuardProfile> wireGuardProfile(const QString &serverId) const;
    caelispect::ErrorCode updateWireGuardProfile(const QString &serverId, const caelispect::WireGuardProfile &profile);

private:
    void ensureDefaultServerValid();
    SecureServersRepository *m_serversRepository;
    SecureAppSettingsRepository *m_appSettingsRepository;
};

#endif // SERVERSCONTROLLER_H
