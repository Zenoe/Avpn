#ifndef APIUTILS_H
#define APIUTILS_H

#include <QNetworkReply>
#include "core/utils/errorCodes.h"

namespace apiUtils
{
    caelispect::ErrorCode checkNetworkReplyErrors(const QList<QSslError> &sslErrors, const QString &replyErrorString,
                                               const QNetworkReply::NetworkError &replyError, const int httpStatusCode,
                                               const QByteArray &responseBody);

}

#endif // APIUTILS_H
