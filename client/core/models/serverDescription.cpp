#include "serverDescription.h"

namespace caelispect
{
ServerDescription buildServerDescription(const WireGuardProfile &profile)
{
    ServerDescription result;
    result.serverName = profile.displayName();
    result.hostName = profile.hostName;
    result.baseDescription = QStringLiteral("WireGuard | ");
    result.collapsedServerDescription = result.baseDescription + result.hostName;
    result.expandedServerDescription = result.hostName;
    return result;
}
} // namespace caelispect
