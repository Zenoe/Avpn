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

    static QScopedPointer<ConfiguratorBase> create(caelispect::Proto protocol);

    virtual caelispect::ProtocolConfig processConfigWithLocalSettings(const caelispect::ConnectionSettings &settings,
                                                                   caelispect::ProtocolConfig protocolConfig);
    virtual caelispect::ProtocolConfig processConfigWithExportSettings(const caelispect::ExportSettings &settings,
                                                                     caelispect::ProtocolConfig protocolConfig);

protected:
    void applyDnsToNativeConfig(const caelispect::DnsSettings &dns, caelispect::ProtocolConfig &protocolConfig);

};

#endif // CONFIGURATORBASE_H
