#include "protocolsModel.h"

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/models/protocols/wireGuardProtocolConfig.h"

using namespace amnezia;

ProtocolsModel::ProtocolsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ProtocolsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_containerConfig.container != DockerContainer::None ? 1 : 0;
}

QHash<int, QByteArray> ProtocolsModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[ProtocolNameRole] = "protocolName";
    roles[ProtocolIndexRole] = "protocolIndex";
    roles[ProtocolStringRole] = "protocolString";
    roles[RawConfigRole] = "rawConfig";
    roles[IsClientProtocolExistsRole] = "isClientProtocolExists";
    roles[IsWireGuardRole] = "isWireGuard";
    roles[IsSftpRole] = "isSftp";
    roles[IsSocks5ProxyRole] = "isSocks5Proxy";
    roles[IsMtProxyRole] = "isMtProxy";
    roles[IsTelemtRole] = "isTelemt";

    return roles;
}

QVariant ProtocolsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return QVariant();
    }

    Proto proto = getProtocolType();
    
    switch (role) {
    case ProtocolNameRole: {
        return amnezia::ProtocolUtils::protocolHumanNames().value(proto);
    }
    case ProtocolIndexRole: return static_cast<int>(proto);
    case ProtocolStringRole: return amnezia::ProtocolUtils::protoToString(proto);
    case IsWireGuardRole: return proto == Proto::WireGuard;
    case IsSftpRole: return proto == Proto::Sftp;
    case IsSocks5ProxyRole: return proto == Proto::Socks5Proxy;
    case IsMtProxyRole: return proto == Proto::MtProxy;
    case IsTelemtRole: return proto == Proto::Telemt;
    case RawConfigRole:
        return getRawConfig();
    case IsClientProtocolExistsRole:
        return isClientProtocolExists();
    }

    return QVariant();
}

void ProtocolsModel::updateModel(const amnezia::ContainerConfig &containerConfig)
{
    beginResetModel();
    m_containerConfig = containerConfig;
    endResetModel();
}

amnezia::Proto ProtocolsModel::getProtocolType() const
{
    return m_containerConfig.getProtocolType();
}

QString ProtocolsModel::getRawConfig() const
{
    QString configString = m_containerConfig.protocolConfig.nativeConfig();
    
    QStringList lines = configString.replace("\r", "").split("\n");
    QString rawConfig;
    for (const QString &l : lines) {
        rawConfig.append(l + "\n");
    }
    return rawConfig;
}

bool ProtocolsModel::isClientProtocolExists() const
{
    return m_containerConfig.protocolConfig.hasClientConfig() && 
           !m_containerConfig.protocolConfig.nativeConfig().isEmpty();
}
