#include "apiUtils.h"

#include <QJsonDocument>
#include <QJsonObject>

using namespace caelispect;

namespace
{
    const QByteArray CAELISPECT_CONFIG_SIGNATURE = QByteArray::fromHex("000000ff");

}

caelispect::ErrorCode apiUtils::checkNetworkReplyErrors(const QList<QSslError> &sslErrors, const QString &replyErrorString,
                                                     const QNetworkReply::NetworkError &replyError, const int httpStatusCode,
                                                     const QByteArray &responseBody)
{
    const int httpStatusCodeNotFound = 404;
    const int httpStatusCodeNotImplemented = 501;
    const int httpStatusCodeTooManyRequests = 429;
    const int httpStatusCodeRequestTimeout = 408;

    if (!sslErrors.empty()) {
        qDebug().noquote() << sslErrors;
        return caelispect::ErrorCode::ApiConfigSslError;
    }
    if (replyError == QNetworkReply::NetworkError::OperationCanceledError
        || replyError == QNetworkReply::NetworkError::TimeoutError) {
        qDebug() << replyError;
        return caelispect::ErrorCode::ApiConfigTimeoutError;
    }
    if (replyError == QNetworkReply::NetworkError::OperationNotImplementedError) {
        qDebug() << replyError;
        return caelispect::ErrorCode::ApiUpdateRequestError;
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseBody);
    if (jsonDoc.isObject()) {
        QJsonObject jsonObj = jsonDoc.object();
        const int httpStatusFromBody = jsonObj.value(QStringLiteral("http_status")).toInt(-1);

        if (httpStatusFromBody == httpStatusCodeTooManyRequests) {
            return caelispect::ErrorCode::ApiRateLimitError;
        }
        if (httpStatusFromBody == httpStatusCodeNotFound) {
            return caelispect::ErrorCode::ApiNotFoundError;
        }
        if (httpStatusFromBody == httpStatusCodeRequestTimeout) {
            return caelispect::ErrorCode::ApiConfigTimeoutError;
        }
        if (httpStatusFromBody == httpStatusCodeNotImplemented) {
            return caelispect::ErrorCode::ApiUpdateRequestError;
        }
        if (httpStatusFromBody >= 300) {
            return caelispect::ErrorCode::ApiConfigDownloadError;
        }
    }

    if (replyError == QNetworkReply::NoError) {
        return caelispect::ErrorCode::NoError;
    }

    qDebug() << "something went wrong";
    return caelispect::ErrorCode::ApiConfigDownloadError;
}
