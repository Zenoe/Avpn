#ifndef SPA_DEVICE_IDENTITY_H
#define SPA_DEVICE_IDENTITY_H

#include <QString>

namespace spa {

struct DeviceIdentity
{
    QString platform;
    QString deviceId;
    QString installationId;

    bool isValid() const
    {
        return !platform.isEmpty() && !deviceId.isEmpty() && !installationId.isEmpty();
    }
};

class DeviceIdentityProvider final
{
public:
    static DeviceIdentity current(QString *errorMessage = nullptr);
};

} // namespace spa

#endif // SPA_DEVICE_IDENTITY_H
