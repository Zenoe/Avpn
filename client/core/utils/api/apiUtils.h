#ifndef APIUTILS_H
#define APIUTILS_H

#include <QNetworkReply>
#include <QObject>

#include "core/utils/serverConfigUtils.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"

namespace apiUtils
{
    bool isSubscriptionExpired(const QString &subscriptionEndDate);

    bool isSubscriptionExpiringSoon(const QString &subscriptionEndDate, int withinDays = 30);

    amnezia::ErrorCode checkNetworkReplyErrors(const QList<QSslError> &sslErrors, const QString &replyErrorString,
                                               const QNetworkReply::NetworkError &replyError, const int httpStatusCode,
                                               const QByteArray &responseBody);

}

#endif // APIUTILS_H
