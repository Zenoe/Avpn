#ifndef SERVERDESCRIPTION_H
#define SERVERDESCRIPTION_H

#include <QString>

#include "core/models/wireguardProfile.h"

namespace caelispect
{
struct ServerDescription
{
    QString serverId;
    QString serverName;
    QString baseDescription;
    QString hostName;
    QString collapsedServerDescription;
    QString expandedServerDescription;
};

ServerDescription buildServerDescription(const WireGuardProfile &profile);
} // namespace caelispect

#endif // SERVERDESCRIPTION_H
