#include "containerUtils.h"

#include <QMetaEnum>
#include <QObject>
#include <QJsonDocument>

using namespace caelispect;

DockerContainer ContainerUtils::containerFromString(const QString &container)
{
    for (DockerContainer c : allContainers()) {
        if (container == containerToString(c))
            return c;
    }
    return DockerContainer::None;
}

QString ContainerUtils::containerToString(DockerContainer c)
{
    if (c == DockerContainer::None)
        return "none";
    QMetaEnum metaEnum = QMetaEnum::fromType<DockerContainer>();
    QString containerKey = metaEnum.valueToKey(static_cast<int>(c));

    return "caelispect-" + containerKey.toLower();
}

QString ContainerUtils::containerTypeToString(DockerContainer c)
{
    if (c == DockerContainer::None)
        return "none";
    QMetaEnum metaEnum = QMetaEnum::fromType<DockerContainer>();
    QString containerKey = metaEnum.valueToKey(static_cast<int>(c));

    return containerKey.toLower();
}

QList<DockerContainer> ContainerUtils::allContainers()
{
    return {
        DockerContainer::None,
        DockerContainer::WireGuard,
        DockerContainer::Dns,
        DockerContainer::Sftp,
        DockerContainer::Socks5Proxy,
        DockerContainer::MtProxy,
        DockerContainer::Telemt,
    };
}

QMap<DockerContainer, QString> ContainerUtils::containerHumanNames()
{
    return { { DockerContainer::None, "Not installed" },
             { DockerContainer::WireGuard, "WireGuard" },
             { DockerContainer::Dns, QObject::tr("CaelispectDNS") },
             { DockerContainer::Sftp, QObject::tr("SFTP file sharing service") },
             { DockerContainer::Socks5Proxy, QObject::tr("SOCKS5 proxy server") },
             { DockerContainer::MtProxy, QObject::tr("MTProxy (Telegram)") },
             { DockerContainer::Telemt, QObject::tr("Telemt (Telegram)") },
    };
}

QMap<DockerContainer, QString> ContainerUtils::containerDescriptions()
{
    return { { DockerContainer::WireGuard,
               QObject::tr("WireGuard - popular VPN protocol with high performance, high speed and low power "
                           "consumption.") },
             { DockerContainer::Dns,
               QObject::tr("Replace the current DNS server with your own. This will increase your privacy level.") },
             { DockerContainer::Sftp,
               QObject::tr("Create a file vault on your server to securely store and transfer files.") },
             { DockerContainer::Socks5Proxy,
               QObject::tr("") },
             { DockerContainer::MtProxy,
               QObject::tr("Telegram MTProto proxy server") },
             { DockerContainer::Telemt,
               QObject::tr("Telegram MTProto proxy (Telemt, Rust)") },
    };
}

QMap<DockerContainer, QString> ContainerUtils::containerDetailedDescriptions()
{
    return {
        { DockerContainer::WireGuard,
          QObject::tr("WireGuard is a modern, streamlined VPN protocol offering stable connectivity and excellent performance across all devices. "
                      "It uses fixed encryption settings, delivering low latency and high data transfer speeds. "
                      "However, WireGuard is easily identifiable by DPI systems due to its distinctive packet signatures, making it susceptible to blocking.\n"
                      "\nFeatures:\n"
                      "* Available on all Caelispect platforms\n"
                      "* Low power consumption on mobile devices\n"
                      "* Minimal configuration required\n"
                      "* Easily detected by DPI systems (susceptible to blocking)\n"
                      "* Operates over UDP protocol") },
        { DockerContainer::Dns, QObject::tr("DNS Service") },
        { DockerContainer::Sftp,
          QObject::tr("After installation, Caelispect will create a\n\n file storage on your server. "
                      "You will be able to access it using\n FileZilla or other SFTP clients, "
                      "as well as mount the disk on your device to access\n it directly from your device.\n\n"
                      "For more detailed information, you can\n find it in the support section under \"Create SFTP file storage.\" ") },
        { DockerContainer::Socks5Proxy, QObject::tr("SOCKS5 proxy server") },
        { DockerContainer::MtProxy,
          QObject::tr("Telegram MTProto proxy server. "
                      "Allows Telegram clients to connect through your server "
                      "using the MTProto protocol. Supports FakeTLS mode for "
                      "bypassing DPI-based blocking.") },
        { DockerContainer::Telemt,
          QObject::tr("Telegram MTProto proxy powered by Telemt (Rust). "
                      "Supports secure and TLS fronting modes with optional traffic masking.") },
    };
}

ServiceType ContainerUtils::containerService(DockerContainer c)
{
    return ProtocolUtils::protocolService(defaultProtocol(c));
}

Proto ContainerUtils::defaultProtocol(DockerContainer c)
{
    switch (c) {
    case DockerContainer::None: return Proto::Unknown;
    case DockerContainer::WireGuard: return Proto::WireGuard;
    case DockerContainer::Dns: return Proto::Dns;
    case DockerContainer::Sftp: return Proto::Sftp;
    case DockerContainer::Socks5Proxy: return Proto::Socks5Proxy;
    case DockerContainer::MtProxy: return Proto::MtProxy;
    case DockerContainer::Telemt: return Proto::Telemt;
    default: return Proto::Unknown;
    }
}

QString ContainerUtils::containerTypeToProtocolString(DockerContainer c)
{
    if (c == DockerContainer::None)
        return "none";

    Proto p = defaultProtocol(c);
    return ProtocolUtils::protoToString(p);
}

bool ContainerUtils::isSupportedByCurrentPlatform(DockerContainer c)
{
#ifdef Q_OS_WINDOWS
    return allContainers().contains(c);

#elif defined(Q_OS_IOS)
    // Standard iOS build (without Network Extension limitations)
    switch (c) {
    case DockerContainer::WireGuard: return true;
    case DockerContainer::MtProxy: return true;
    case DockerContainer::Telemt: return true;
    default:
        return false;
    }

#elif defined(MACOS_NE)
    switch (c) {
    case DockerContainer::WireGuard: return true;
    case DockerContainer::MtProxy: return true;
    case DockerContainer::Telemt: return true;
    default:
        return false;
    }
#elif defined(Q_OS_MAC)
    return allContainers().contains(c);

#elif defined(Q_OS_ANDROID)
    switch (c) {
    case DockerContainer::WireGuard: return true;
    case DockerContainer::MtProxy: return true;
    case DockerContainer::Telemt: return true;
    default: return false;
    }

#elif defined(Q_OS_LINUX)
    return allContainers().contains(c);

#else
    return false;
#endif
}

QStringList ContainerUtils::fixedPortsForContainer(DockerContainer c)
{
    Q_UNUSED(c)
    return {};
}

bool ContainerUtils::isEasySetupContainer(DockerContainer container)
{
    switch (container) {
    default: return false;
    }
}

QString ContainerUtils::easySetupHeader(DockerContainer container)
{
    switch (container) {
    default: return "";
    }
}

QString ContainerUtils::easySetupDescription(DockerContainer container)
{
    switch (container) {
    default: return "";
    }
}

int ContainerUtils::easySetupOrder(DockerContainer container)
{
    switch (container) {
    default: return 0;
    }
}

bool ContainerUtils::isShareable(DockerContainer container)
{
    if (isUnsupportedContainer(container)) {
        return false;
    }

    switch (container) {
    case DockerContainer::Dns: return false;
    case DockerContainer::Sftp: return false;
    case DockerContainer::Socks5Proxy: return false;
    case DockerContainer::MtProxy: return false;
    case DockerContainer::Telemt: return false;
    default: return true;
    }
}

bool ContainerUtils::isUnsupportedContainer(DockerContainer container)
{
    return !allContainers().contains(container);
}

QJsonObject ContainerUtils::getProtocolConfigFromContainer(const Proto protocol, const QJsonObject &containerConfig)
{
    QString protocolConfigString = containerConfig.value(ProtocolUtils::protoToString(protocol))
    .toObject()
            .value(configKey::lastConfig)
            .toString();

    return QJsonDocument::fromJson(protocolConfigString.toUtf8()).object();
}

int ContainerUtils::installPageOrder(DockerContainer container)
{
    switch (container) {
    case DockerContainer::WireGuard: return 1;
    case DockerContainer::MtProxy:
    case DockerContainer::Telemt:
        return 20;
    default: return 0;
    }
}

