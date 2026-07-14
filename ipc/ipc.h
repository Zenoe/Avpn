#ifndef IPC_H
#define IPC_H

#include <QObject>
#include <QString>

#include "../client/core/utils/utilities.h"

#define IPC_SERVICE_URL "local:AmneziaVpnIpcInterface"

namespace amnezia {

enum PermittedProcess {
    Invalid,
    Wireguard
};

inline QString permittedProcessPath(PermittedProcess pid)
{
    switch (pid) {
        case PermittedProcess::Wireguard:
            return Utils::wireguardExecPath();
        default:
            return "";
    }
}


inline QString getIpcServiceUrl() {
#ifdef Q_OS_WIN
    return IPC_SERVICE_URL;
#else
    return QString("/tmp/%1").arg(IPC_SERVICE_URL);
#endif
}

inline QString getIpcProcessUrl(int pid) {
#ifdef Q_OS_WIN
    return QString("%1_%2").arg(IPC_SERVICE_URL).arg(pid);
#else
    return QString("/tmp/%1_%2").arg(IPC_SERVICE_URL).arg(pid);
#endif
}

inline QStringList sanitizeArguments(PermittedProcess proc, const QStringList &args) {
    Q_UNUSED(proc)
    return args;
}

} // namespace amnezia

#endif // IPC_H
