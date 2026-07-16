#ifndef CONFIGURATORBASE_H
#define CONFIGURATORBASE_H

#include <QObject>
#include <QScopedPointer>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/models/containerConfig.h"
#include "core/models/protocolConfig.h"

class ConfiguratorBase : public QObject
{
    Q_OBJECT
public:
    explicit ConfiguratorBase(QObject *parent = nullptr);

    static QScopedPointer<ConfiguratorBase> create(amnezia::Proto protocol);

    virtual amnezia::ProtocolConfig processConfigWithLocalSettings(const amnezia::ConnectionSettings &settings,
                                                                   amnezia::ProtocolConfig protocolConfig);
    virtual amnezia::ProtocolConfig processConfigWithExportSettings(const amnezia::ExportSettings &settings,
                                                                     amnezia::ProtocolConfig protocolConfig);

protected:
    void applyDnsToNativeConfig(const amnezia::DnsSettings &dns, amnezia::ProtocolConfig &protocolConfig);

};

#endif // CONFIGURATORBASE_H
