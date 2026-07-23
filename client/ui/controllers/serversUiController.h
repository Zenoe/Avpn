#ifndef SERVERSUICONTROLLER_H
#define SERVERSUICONTROLLER_H

#include <QObject>
#include <QVector>

#include "core/controllers/serversController.h"
#include "core/controllers/settingsController.h"
#include "ui/models/protocols/wireguardConfigModel.h"
#include "ui/models/serversModel.h"

class ServersUiController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString defaultServerId READ getDefaultServerId NOTIFY defaultServerIdChanged)
    Q_PROPERTY(QString defaultServerName READ getDefaultServerName NOTIFY defaultServerIdChanged)
    Q_PROPERTY(QString defaultServerDefaultContainerName READ getDefaultServerDefaultContainerName NOTIFY defaultServerIdChanged)
    Q_PROPERTY(QString defaultServerDescriptionCollapsed READ getDefaultServerDescriptionCollapsed NOTIFY defaultServerIdChanged)
    Q_PROPERTY(QString defaultServerImagePathCollapsed READ getDefaultServerImagePathCollapsed NOTIFY defaultServerIdChanged)
    Q_PROPERTY(QString defaultServerDescriptionExpanded READ getDefaultServerDescriptionExpanded NOTIFY defaultServerIdChanged)
    Q_PROPERTY(bool isDefaultServerDefaultContainerHasSplitTunneling READ isDefaultServerDefaultContainerHasSplitTunneling NOTIFY defaultServerIdChanged)
    Q_PROPERTY(QString processedServerId READ getProcessedServerId WRITE setProcessedServerId NOTIFY processedServerIdChanged)
public:
    explicit ServersUiController(ServersController *serversController, SettingsController *settingsController,
                                 ServersModel *serversModel, WireGuardConfigModel *wireGuardConfigModel, QObject *parent = nullptr);
public slots:
    void removeServer(const QString &serverId);
    void removeServerAtIndex(int index);
    void editServerName(const QString &serverId, const QString &name);
    void setDefaultServer(const QString &serverId);
    void setDefaultServerAtIndex(int index);
    void openClientProtocolSettings(const QString &serverId);
    void saveClientProtocolSettings(const QString &serverId);
    bool isDefaultServerCurrentlyProcessed() const;
    void onDefaultServerChanged(const QString &defaultServerId);
    void setProcessedServerId(const QString &serverId);
public:
    QString getDefaultServerId() const;
    QString getDefaultServerName() const;
    QString getDefaultServerDefaultContainerName() const;
    QString getDefaultServerDescriptionCollapsed() const;
    QString getDefaultServerImagePathCollapsed() const;
    QString getDefaultServerDescriptionExpanded() const;
    bool isDefaultServerDefaultContainerHasSplitTunneling() const;
    QString serverName(const QString &serverId) const;
    QString serverHostName(const QString &serverId) const;
    QString getProcessedServerId() const;
    QString getServerId(int index) const;
    int getServerIndexById(const QString &serverId) const;
    int getServersCount() const;
signals:
    void errorOccurred(const QString &errorMessage);
    void finished(const QString &message);
    void defaultServerIdChanged(const QString &serverId);
    void processedServerIdChanged(const QString &serverId);
public:
    void updateModel();
private:
    const caelispect::ServerDescription &description(const QString &serverId) const;
    ServersController *m_serversController;
    SettingsController *m_settingsController;
    ServersModel *m_serversModel;
    WireGuardConfigModel *m_wireGuardConfigModel;
    QVector<caelispect::ServerDescription> m_descriptions;
    QString m_processedServerId;
};

#endif // SERVERSUICONTROLLER_H
