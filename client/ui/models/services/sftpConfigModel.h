#ifndef SFTPCONFIGMODEL_H
#define SFTPCONFIGMODEL_H

#include <QAbstractListModel>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/protocols/sftpProtocolConfig.h"

class SftpConfigModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        PortRole = Qt::UserRole + 1,
        UserNameRole,
        PasswordRole
    };

    explicit SftpConfigModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(caelispect::DockerContainer container, const caelispect::SftpProtocolConfig &protocolConfig);
    caelispect::SftpProtocolConfig getProtocolConfig();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    caelispect::DockerContainer m_container;
    caelispect::SftpProtocolConfig m_protocolConfig;
    
    void applyDefaults(caelispect::SftpProtocolConfig& config);
};

#endif // SFTPCONFIGMODEL_H
