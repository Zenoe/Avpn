#include "wireguardConfigModel.h"

#include "core/utils/constants/protocolConstants.h"

using namespace caelispect;

WireGuardConfigModel::WireGuardConfigModel(QObject *parent) : QAbstractListModel(parent) {}
int WireGuardConfigModel::rowCount(const QModelIndex &parent) const { Q_UNUSED(parent); return 1; }

bool WireGuardConfigModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() != 0 || role != ClientMtuRole) return false;
    m_config.mtu = value.toString();
    emit dataChanged(index, index, { role });
    return true;
}

QVariant WireGuardConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() != 0) return {};
    if (role == ClientMtuRole) return m_config.mtu;
    if (role == PortRole) return m_config.port;
    return {};
}

void WireGuardConfigModel::updateModel(const WireGuardClientConfig &config)
{
    beginResetModel();
    m_config = config;
    applyDefaults(m_config);
    endResetModel();
}

WireGuardClientConfig WireGuardConfigModel::getConfig() const { return m_config; }
void WireGuardConfigModel::applyDefaults(WireGuardClientConfig &config)
{
    if (config.mtu.isEmpty()) config.mtu = protocols::wireguard::defaultMtu;
}

QHash<int, QByteArray> WireGuardConfigModel::roleNames() const
{
    return { { PortRole, "port" }, { ClientMtuRole, "clientMtu" } };
}
