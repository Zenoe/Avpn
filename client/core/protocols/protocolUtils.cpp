#include "protocolUtils.h"

#include <QRandomGenerator>
#include <QObject>

using namespace amnezia;

QList<Proto> ProtocolUtils::allProtocols()
{
    return {
        Proto::Unknown,
        Proto::WireGuard,
        Proto::Dns,
        Proto::Sftp,
        Proto::Socks5Proxy,
        Proto::MtProxy,
        Proto::Telemt,
    };
}

TransportProto ProtocolUtils::transportProtoFromString(QString p)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<TransportProto>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        TransportProto tp = static_cast<TransportProto>(i);
        if (p.toLower() == transportProtoToString(tp).toLower())
            return tp;
    }
    return TransportProto::Udp;
}

QString ProtocolUtils::transportProtoToString(TransportProto proto, Proto p)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<TransportProto>();
    QString protoKey = metaEnum.valueToKey(static_cast<int>(proto));
    return protoKey.toLower();
}

Proto ProtocolUtils::protoFromString(QString proto)
{
    for (Proto p : allProtocols()) {
        if (proto == protoToString(p))
            return p;
    }
    return Proto::Unknown;
}

QString ProtocolUtils::protoToString(Proto p)
{
    if (p == Proto::Unknown)
        return "";

    QMetaEnum metaEnum = QMetaEnum::fromType<Proto>();
    QString protoKey = metaEnum.valueToKey(static_cast<int>(p));
    return protoKey.toLower();
}

QMap<Proto, QString> ProtocolUtils::protocolHumanNames()
{
    return { { Proto::WireGuard, "WireGuard" },
             { Proto::Dns, "DNS Service" },
             { Proto::Sftp, QObject::tr("SFTP service") },
             { Proto::Socks5Proxy, QObject::tr("SOCKS5 proxy server") },
             { Proto::MtProxy, QObject::tr("MTProxy (Telegram)") },
             { Proto::Telemt, QObject::tr("Telemt (Telegram)") },
    };
}

QMap<Proto, QString> ProtocolUtils::protocolDescriptions()
{
    return {};
}

ServiceType ProtocolUtils::protocolService(Proto p)
{
    switch (p) {
    case Proto::Unknown: return ServiceType::None;
    case Proto::WireGuard: return ServiceType::Vpn;
    case Proto::Dns: return ServiceType::Other;
    case Proto::Sftp: return ServiceType::Other;
    case Proto::Socks5Proxy: return ServiceType::Other;
    case Proto::MtProxy: return ServiceType::Other;
    case Proto::Telemt: return ServiceType::Other;
    default: return ServiceType::Other;
    }
}

int ProtocolUtils::getPortForInstall(Proto p)
{
    switch (p) {
    case WireGuard:
    case Socks5Proxy:
        return QRandomGenerator::global()->bounded(30000, 50000);
    case MtProxy:
    case Telemt:
    default:
        return defaultPort(p);
    }
}

int ProtocolUtils::defaultPort(Proto p)
{
    switch (p) {
    case Proto::Unknown: return -1;
    case Proto::WireGuard: return QString(protocols::wireguard::defaultPort).toInt();
    case Proto::Dns: return 53;
    case Proto::Sftp: return 222;
    case Proto::Socks5Proxy: return 38080;
    case Proto::MtProxy: return QString(protocols::mtProxy::defaultPort).toInt();
    case Proto::Telemt: return QString(protocols::telemt::defaultPort).toInt();
    default: return -1;
    }
}

bool ProtocolUtils::defaultPortChangeable(Proto p)
{
    switch (p) {
    case Proto::Unknown: return false;
    case Proto::WireGuard: return true;
    case Proto::Dns: return false;
    case Proto::Sftp: return true;
    case Proto::Socks5Proxy: return true;
    case Proto::MtProxy: return true;
    case Proto::Telemt: return true;
    default: return false;
    }
}

TransportProto ProtocolUtils::defaultTransportProto(Proto p)
{
    switch (p) {
    case Proto::Unknown: return TransportProto::Udp;
    case Proto::WireGuard: return TransportProto::Udp;
    case Proto::Dns: return TransportProto::Udp;
    case Proto::Sftp: return TransportProto::Tcp;
    case Proto::Socks5Proxy: return TransportProto::Tcp;
    case Proto::MtProxy: return TransportProto::Tcp;
    case Proto::Telemt: return TransportProto::Tcp;
    default: return TransportProto::Udp;
    }
}

bool ProtocolUtils::defaultTransportProtoChangeable(Proto p)
{
    switch (p) {
    case Proto::Unknown: return false;
    case Proto::WireGuard: return false;
    case Proto::Dns: return false;
    case Proto::Sftp: return false;
    case Proto::Socks5Proxy: return false;
    case Proto::MtProxy: return false;
    case Proto::Telemt: return false;
    default: return false;
    }
}

QString ProtocolUtils::key_proto_config_data(Proto p)
{
    return protoToString(p) + "_config_data";
}

QString ProtocolUtils::key_proto_config_path(Proto p)
{
    return protoToString(p) + "_config_path";
}
