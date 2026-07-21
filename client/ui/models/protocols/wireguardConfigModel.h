#ifndef WIREGUARDCONFIGMODEL_H
#define WIREGUARDCONFIGMODEL_H

#include <QAbstractListModel>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/protocols/wireGuardProtocolConfig.h"

class WireGuardConfigModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        SubnetAddressRole = Qt::UserRole + 1,
        PortRole,
        ClientMtuRole
    };

    explicit WireGuardConfigModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(caelispect::DockerContainer container, const caelispect::WireGuardProtocolConfig &protocolConfig);
    caelispect::WireGuardProtocolConfig getProtocolConfig();

    bool isServerSettingsEqual();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    caelispect::DockerContainer m_container;
    caelispect::WireGuardProtocolConfig m_protocolConfig;
    caelispect::WireGuardProtocolConfig m_originalProtocolConfig;
    
    void applyDefaultsToServerConfig(caelispect::WireGuardServerConfig& config);
    void applyDefaultsToClientConfig(caelispect::WireGuardClientConfig& config);
};

#endif // WIREGUARDCONFIGMODEL_H
