#ifndef PROTOCOLS_MODEL_H
#define PROTOCOLS_MODEL_H

#include <QAbstractListModel>

#include "core/models/containerConfig.h"

class ProtocolsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        ProtocolNameRole = Qt::UserRole + 1,
        ProtocolIndexRole,
        ProtocolStringRole,
        RawConfigRole,
        IsClientProtocolExistsRole,
        // Protocol type check roles
        IsWireGuardRole,
        IsSftpRole,
        IsSocks5ProxyRole,
        IsMtProxyRole,
        IsTelemtRole,
    };

    explicit ProtocolsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(const caelispect::ContainerConfig &containerConfig);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    caelispect::Proto getProtocolType() const;
    QString getRawConfig() const;
    bool isClientProtocolExists() const;

    caelispect::ContainerConfig m_containerConfig;
};

#endif // PROTOCOLS_MODEL_H
