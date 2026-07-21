#include "spa/spaDeviceIdentity.h"

#include <QSysInfo>

#include "spaCrypto.h"

namespace {

QString currentPlatform()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("macos");
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    return QStringLiteral("linux");
#else
    return {};
#endif
}

QString deriveIdentifier(const QByteArray &domain, const QString &platform, const QByteArray &machineId)
{
    QByteArray input = domain;
    input.append('\0');
    input.append(platform.toUtf8());
    input.append('\0');
    input.append(machineId.trimmed().toLower());
    return QString::fromLatin1(spa::crypto::sm3(input).toHex());
}

} // namespace

namespace spa {

DeviceIdentity DeviceIdentityProvider::current(QString *errorMessage)
{
    DeviceIdentity result;
    result.platform = currentPlatform();
    const QByteArray machineId = QSysInfo::machineUniqueId();
    if (result.platform.isEmpty() || machineId.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Unable to obtain a stable machine identifier on this platform");
        }
        return {};
    }

    result.deviceId = deriveIdentifier(QByteArrayLiteral("ASPA-DEVICE-v1"), result.platform, machineId);
    result.installationId = deriveIdentifier(QByteArrayLiteral("ASPA-INSTALLATION-v1"), result.platform, machineId);
    return result;
}

} // namespace spa
