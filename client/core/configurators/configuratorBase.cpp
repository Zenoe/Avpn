#include "configuratorBase.h"

#include "core/configurators/awgConfigurator.h"
#include "core/configurators/wireguardConfigurator.h"

using namespace amnezia;

ConfiguratorBase::ConfiguratorBase(SshSession* sshSession, QObject *parent)
    : QObject { parent }, m_sshSession(sshSession)
{
}

QScopedPointer<ConfiguratorBase> ConfiguratorBase::create(Proto protocol,
                                                          SshSession* sshSession)
{
    switch (protocol) {
    case Proto::WireGuard: return QScopedPointer<ConfiguratorBase>(new WireguardConfigurator(sshSession, false));
    case Proto::Awg: return QScopedPointer<ConfiguratorBase>(new AwgConfigurator(sshSession));
    default: return QScopedPointer<ConfiguratorBase>();
    }
}

ProtocolConfig ConfiguratorBase::processConfigWithLocalSettings(const ConnectionSettings &settings,
                                                                 ProtocolConfig protocolConfig)
{
    applyDnsToNativeConfig(settings.dns, protocolConfig);
    return protocolConfig;
}

ProtocolConfig ConfiguratorBase::processConfigWithExportSettings(const ExportSettings &settings,
                                                                 ProtocolConfig protocolConfig)
{
    applyDnsToNativeConfig(settings.dns, protocolConfig);
    return protocolConfig;
}

void ConfiguratorBase::applyDnsToNativeConfig(const DnsSettings &dns, ProtocolConfig &protocolConfig)
{
    QString config = protocolConfig.nativeConfig();
    config.replace("$PRIMARY_DNS", dns.primaryDns);
    config.replace("$SECONDARY_DNS", dns.secondaryDns);
    protocolConfig.setNativeConfig(config);
}
