#ifndef PROTOCOLENUM_H
#define PROTOCOLENUM_H

#include <QMetaEnum>
#include <QObject>

namespace caelispect
{
    namespace ProtocolEnumNS
    {
        Q_NAMESPACE

        enum TransportProto {
            Udp,
            Tcp,
            TcpAndUdp
        };
        Q_ENUM_NS(TransportProto)

        enum Proto {
            Unknown = 0,
            WireGuard = 1,
        };
        Q_ENUM_NS(Proto)

    } // namespace ProtocolEnumNS

    using namespace ProtocolEnumNS;
}

#endif // PROTOCOLENUM_H


