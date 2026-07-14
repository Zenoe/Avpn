#include "protocolConfig.h"

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/protocols/dnsProtocolConfig.h"
#include "core/models/protocols/mtProxyProtocolConfig.h"
#include "core/models/protocols/telemtProtocolConfig.h"

namespace amnezia
{

using namespace ProtocolEnumNS;
using namespace ProtocolUtils;

Proto ProtocolConfig::type() const
{
    return std::visit([](auto&& arg) -> Proto {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig>) {
            return Proto::Awg;
        } else if constexpr (std::is_same_v<T, WireGuardProtocolConfig>) {
            return Proto::WireGuard;
        } else if constexpr (std::is_same_v<T, SftpProtocolConfig>) {
            return Proto::Sftp;
        } else if constexpr (std::is_same_v<T, Socks5ProxyProtocolConfig>) {
            return Proto::Socks5Proxy;
        } else if constexpr (std::is_same_v<T, DnsProtocolConfig>) {
            return Proto::Dns;
        } else if constexpr (std::is_same_v<T, MtProxyProtocolConfig>) {
            return Proto::MtProxy;
        } else if constexpr (std::is_same_v<T, TelemtProtocolConfig>) {
            return Proto::Telemt;
        }
        return Proto::Unknown;
    }, data);
}

QString ProtocolConfig::port() const
{
    return std::visit([](auto&& arg) -> QString {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig>) {
            return arg.serverConfig.port;
        } else if constexpr (std::is_same_v<T, WireGuardProtocolConfig>) {
            return arg.serverConfig.port;
        } else if constexpr (std::is_same_v<T, SftpProtocolConfig>) {
            return arg.port;
        } else if constexpr (std::is_same_v<T, Socks5ProxyProtocolConfig>) {
            return arg.port;
        } else if constexpr (std::is_same_v<T, DnsProtocolConfig>) {
            return QString();
        } else if constexpr (std::is_same_v<T, MtProxyProtocolConfig>) {
            return arg.port.isEmpty() ? QString(protocols::mtProxy::defaultPort) : arg.port;
        } else if constexpr (std::is_same_v<T, TelemtProtocolConfig>) {
            return arg.port.isEmpty() ? QString(protocols::telemt::defaultPort) : arg.port;
        }
        return QString();
    }, data);
}

QString ProtocolConfig::transportProto() const
{
    return std::visit([](auto&& arg) -> QString {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig>) {
            return arg.serverConfig.transportProto;
        } else if constexpr (std::is_same_v<T, WireGuardProtocolConfig>) {
            return arg.serverConfig.transportProto;
        } else if constexpr (std::is_same_v<T, DnsProtocolConfig>) {
            return QString();
        } else if constexpr (std::is_same_v<T, MtProxyProtocolConfig>) {
            return QStringLiteral("tcp");
        } else if constexpr (std::is_same_v<T, TelemtProtocolConfig>) {
            return QStringLiteral("tcp");
        }
        return QString();
    }, data);
}

bool ProtocolConfig::hasClientConfig() const
{
    return std::visit([](auto&& arg) -> bool {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig> ||
                      std::is_same_v<T, WireGuardProtocolConfig>) {
            return arg.hasClientConfig();
        }
        return false;
    }, data);
}

QString ProtocolConfig::clientId() const
{
    return std::visit([](auto&& arg) -> QString {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig>) {
            if (arg.clientConfig.has_value()) {
                return arg.clientConfig->clientId;
            }
        } else if constexpr (std::is_same_v<T, WireGuardProtocolConfig>) {
            if (arg.clientConfig.has_value()) {
                return arg.clientConfig->clientId;
            }
        }
        return QString();
    }, data);
}

QJsonObject ProtocolConfig::getClientConfigJson() const
{
    return std::visit([](auto&& arg) -> QJsonObject {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig>) {
            if (arg.hasClientConfig()) {
                return arg.clientConfig->toJson();
            }
        } else if constexpr (std::is_same_v<T, WireGuardProtocolConfig>) {
            if (arg.hasClientConfig()) {
                return arg.clientConfig->toJson();
            }
        }
        return QJsonObject();
    }, data);
}

void ProtocolConfig::setClientConfigJson(const QJsonObject& json)
{
    std::visit([&json](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig>) {
            arg.setClientConfig(AwgClientConfig::fromJson(json));
        } else if constexpr (std::is_same_v<T, WireGuardProtocolConfig>) {
            arg.setClientConfig(WireGuardClientConfig::fromJson(json));
        }
    }, data);
}

void ProtocolConfig::clearClientConfig()
{
    std::visit([](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig> ||
                      std::is_same_v<T, WireGuardProtocolConfig>) {
            arg.clearClientConfig();
        }
    }, data);
}

QString ProtocolConfig::nativeConfig() const
{
    return std::visit([](auto&& arg) -> QString {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig>) {
            if (arg.clientConfig.has_value()) {
                return arg.clientConfig->nativeConfig;
            }
        } else if constexpr (std::is_same_v<T, WireGuardProtocolConfig>) {
            if (arg.clientConfig.has_value()) {
                return arg.clientConfig->nativeConfig;
            }
        }
        return QString();
    }, data);
}

void ProtocolConfig::setNativeConfig(const QString &config)
{
    std::visit([&config](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig>) {
            if (arg.clientConfig.has_value()) {
                arg.clientConfig->nativeConfig = config;
            }
        } else if constexpr (std::is_same_v<T, WireGuardProtocolConfig>) {
            if (arg.clientConfig.has_value()) {
                arg.clientConfig->nativeConfig = config;
            }
        }
    }, data);
}

bool ProtocolConfig::isThirdPartyConfig() const
{
    return std::visit([](auto&& arg) -> bool {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig> ||
                      std::is_same_v<T, WireGuardProtocolConfig>) {
            return arg.serverConfig.isThirdPartyConfig;
        }
        return false;
    }, data);
}

QJsonObject ProtocolConfig::toJson() const
{
    return std::visit([](auto&& arg) -> QJsonObject {
        return arg.toJson();
    }, data);
}

ProtocolConfig ProtocolConfig::fromJson(const QJsonObject& json, Proto type)
{
    switch (type) {
    case Proto::Awg:
        return ProtocolConfig{AwgProtocolConfig::fromJson(json)};
    case Proto::WireGuard:
        return ProtocolConfig{WireGuardProtocolConfig::fromJson(json)};
    case Proto::Sftp:
        return ProtocolConfig{SftpProtocolConfig::fromJson(json)};
    case Proto::Socks5Proxy:
        return ProtocolConfig{Socks5ProxyProtocolConfig::fromJson(json)};
    case Proto::Dns:
        return ProtocolConfig{DnsProtocolConfig::fromJson(json)};
    case Proto::MtProxy:
        return ProtocolConfig{MtProxyProtocolConfig::fromJson(json)};
    case Proto::Telemt:
        return ProtocolConfig{TelemtProtocolConfig::fromJson(json)};
    default:
        return ProtocolConfig{AwgProtocolConfig{}};
    }
}

} // namespace amnezia
