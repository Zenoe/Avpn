#include "protocolUtils.h"

#include <QObject>

using namespace caelispect;

QList<Proto> ProtocolUtils::allProtocols() { return { Proto::Unknown, Proto::WireGuard }; }
TransportProto ProtocolUtils::transportProtoFromString(QString value)
{
    value = value.toLower();
    return value == QStringLiteral("tcp") ? TransportProto::Tcp : TransportProto::Udp;
}
QString ProtocolUtils::transportProtoToString(TransportProto proto, Proto) { return proto == TransportProto::Tcp ? QStringLiteral("tcp") : QStringLiteral("udp"); }
Proto ProtocolUtils::protoFromString(QString value) { return value.compare(QStringLiteral("wireguard"), Qt::CaseInsensitive) == 0 ? Proto::WireGuard : Proto::Unknown; }
QString ProtocolUtils::protoToString(Proto proto) { return proto == Proto::WireGuard ? QStringLiteral("wireguard") : QString(); }
QMap<Proto, QString> ProtocolUtils::protocolHumanNames() { return { { Proto::WireGuard, QStringLiteral("WireGuard") } }; }
QMap<Proto, QString> ProtocolUtils::protocolDescriptions() { return {}; }
QString ProtocolUtils::key_proto_config_data(Proto proto) { return protoToString(proto) + QStringLiteral("_config_data"); }
TransportProto ProtocolUtils::defaultTransportProto(Proto) { return TransportProto::Udp; }
