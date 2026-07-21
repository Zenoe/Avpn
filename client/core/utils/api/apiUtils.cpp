#include "apiUtils.h"

#include "core/utils/serverConfigUtils.h"
#include "core/utils/constants/configKeys.h"
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

using namespace caelispect;

namespace
{
    const QByteArray CAELISPECT_CONFIG_SIGNATURE = QByteArray::fromHex("000000ff");

    constexpr QLatin1String unprocessableSubscriptionMessage("Failed to retrieve subscription information. Is it activated?");
    constexpr QLatin1String trialAlreadyUsedMessage("trial subscription already used");

    QDateTime subscriptionEndUtcFromString(const QString &subscriptionEndDate)
    {
        if (subscriptionEndDate.isEmpty()) {
            return {};
        }
        QDateTime endDate = QDateTime::fromString(subscriptionEndDate, Qt::ISODateWithMs).toUTC();
        if (!endDate.isValid()) {
            endDate = QDateTime::fromString(subscriptionEndDate, Qt::ISODate).toUTC();
        }
        return endDate;
    }

    QString apiErrorMessageFromJson(const QJsonObject &jsonObj)
    {
        const QJsonValue value = jsonObj.value(QStringLiteral("message"));
        return value.isString() ? value.toString().trimmed() : QString();
    }

    QString escapeUnicode(const QString &input)
    {
        QString output;
        for (QChar c : input) {
            if (c.unicode() < 0x20 || c.unicode() > 0x7E) {
                output += QString("\\u%1").arg(QString::number(c.unicode(), 16).rightJustified(4, '0'));
            } else {
                output += c;
            }
        }
        return output;
    }
}

bool apiUtils::isSubscriptionExpired(const QString &subscriptionEndDate)
{
    if (subscriptionEndDate.isEmpty()) {
        return false;
    }
    const QDateTime endDate = subscriptionEndUtcFromString(subscriptionEndDate);
    if (!endDate.isValid()) {
        return false;
    }
    return endDate <= QDateTime::currentDateTimeUtc();
}

bool apiUtils::isSubscriptionExpiringSoon(const QString &subscriptionEndDate, int withinDays)
{
    if (subscriptionEndDate.isEmpty()) {
        return false;
    }
    const QDateTime endDate = subscriptionEndUtcFromString(subscriptionEndDate);
    if (!endDate.isValid()) {
        return false;
    }
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    if (endDate <= nowUtc) {
        return false;
    }
    return endDate <= nowUtc.addDays(withinDays);
}

caelispect::ErrorCode apiUtils::checkNetworkReplyErrors(const QList<QSslError> &sslErrors, const QString &replyErrorString,
                                                     const QNetworkReply::NetworkError &replyError, const int httpStatusCode,
                                                     const QByteArray &responseBody)
{
    const int httpStatusCodeConflict = 409;
    const int httpStatusCodeNotFound = 404;
    const int httpStatusCodeNotImplemented = 501;
    const int httpStatusCodePaymentRequired = 402;
    const int httpStatusCodeTooManyRequests = 429;
    const int httpStatusCodeRequestTimeout = 408;
    const int httpStatusCodeUnprocessableEntity = 422;

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
        if (httpStatusFromBody == httpStatusCodeConflict) {
            if (apiErrorMessageFromJson(jsonObj).contains(trialAlreadyUsedMessage, Qt::CaseInsensitive)) {
                return caelispect::ErrorCode::ApiTrialAlreadyUsedError;
            }
            return caelispect::ErrorCode::ApiConfigLimitError;
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
        if (httpStatusFromBody == httpStatusCodeUnprocessableEntity) {
            if (apiErrorMessageFromJson(jsonObj) == unprocessableSubscriptionMessage) {
                return caelispect::ErrorCode::ApiSubscriptionExpiredError;
            }
            return caelispect::ErrorCode::ApiConfigDownloadError;
        }
        if (httpStatusFromBody == httpStatusCodePaymentRequired) {
            const QString message = apiErrorMessageFromJson(jsonObj);
            if (message.contains(QLatin1String("refresh_captcha"), Qt::CaseInsensitive)) {
                return caelispect::ErrorCode::ApiCaptchaRefreshError;
            }
            if (message.contains(QLatin1String("invalid_captcha"), Qt::CaseInsensitive)) {
                return caelispect::ErrorCode::ApiCaptchaInvalidError;
            }
            if (jsonObj.contains(QStringLiteral("captcha_id")) || jsonObj.contains(QStringLiteral("captcha_image"))
                || message.compare(QLatin1String("rate_limit_exceeded"), Qt::CaseInsensitive) == 0
                || message.contains(QLatin1String("rate_limit_exceeded"), Qt::CaseInsensitive)) {
                return caelispect::ErrorCode::ApiCaptchaRequiredError;
            }
            return caelispect::ErrorCode::ApiSubscriptionNotActiveError;
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
