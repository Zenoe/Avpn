#ifndef CONTAINERENUM_H
#define CONTAINERENUM_H

#include <QMetaEnum>
#include <QObject>

namespace caelispect
{
    namespace ContainerEnumNS
    {
        Q_NAMESPACE
        enum DockerContainer {
            None = 0,
            // Preserve the persisted value used by existing WireGuard profiles.
            WireGuard = 3,

            // Preserve persisted numeric values for retained non-VPN services.
            Dns = 11,
            Sftp = 12,
            Socks5Proxy = 13,
            MtProxy = 14,
            Telemt = 15,
        };
        Q_ENUM_NS(DockerContainer)
    } // namespace ContainerEnumNS

    using namespace ContainerEnumNS;
}

#endif // CONTAINERENUM_H


