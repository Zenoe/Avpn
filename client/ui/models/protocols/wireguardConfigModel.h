#ifndef WIREGUARDCONFIGMODEL_H
#define WIREGUARDCONFIGMODEL_H

#include <QAbstractListModel>

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
    void updateModel(const caelispect::WireGuardClientConfig &config);
    caelispect::WireGuardClientConfig getConfig() const;

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    caelispect::WireGuardClientConfig m_config;
    void applyDefaults(caelispect::WireGuardClientConfig &config);
};

#endif // WIREGUARDCONFIGMODEL_H
