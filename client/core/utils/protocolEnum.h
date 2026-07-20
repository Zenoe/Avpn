#ifndef PROTOCOLENUM_H
#define PROTOCOLENUM_H

#include <QMetaEnum>
#include <QObject>

namespace amnezia
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
            // Preserve the persisted value used by existing WireGuard profiles.
            WireGuard = 2,

            // Preserve persisted numeric values for retained non-VPN services.
            Dns = 8,
            Sftp = 9,
            Socks5Proxy = 10,
            MtProxy = 11,
            Telemt = 12,
        };
        Q_ENUM_NS(Proto)

        enum ServiceType {
            None = 0,
            Vpn,
            Other
        };
        Q_ENUM_NS(ServiceType)
    } // namespace ProtocolEnumNS

    using namespace ProtocolEnumNS;
}

#endif // PROTOCOLENUM_H


