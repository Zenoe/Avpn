#ifndef TELEMTDIAGNOSTICS_H
#define TELEMTDIAGNOSTICS_H

#include "containerDiagnostics.h"

#include <QString>

namespace caelispect
{
    struct TelemtDiagnostics : ContainerDiagnostics
    {
        bool upstreamReachable = false;
        int clientsConnected = -1;
        QString lastConfigRefresh;
        QString statsEndpoint;
    };

} // namespace caelispect

#endif // TELEMTDIAGNOSTICS_H
