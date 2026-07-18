#ifndef SPA_TYPES_H
#define SPA_TYPES_H

#include <QByteArray>
#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>

#include <utility>

namespace spa {

struct Request
{
    QByteArray requestId;
    QDateTime createdAtUtc;
    QVariantMap attributes;
};

struct LoginEndpoint
{
    QString scheme = QStringLiteral("https");
    QString host;
    quint16 port = 0;
    QByteArray ticket;
    QDateTime expiresAtUtc;

    bool isStructurallyValid() const
    {
        return !scheme.isEmpty() && !host.isEmpty() && port != 0 && !ticket.isEmpty()
                && expiresAtUtc.isValid();
    }

    bool isExpired(const QDateTime &nowUtc = QDateTime::currentDateTimeUtc()) const
    {
        return !expiresAtUtc.isValid() || expiresAtUtc <= nowUtc;
    }

    QUrl url(const QString &path = {}) const
    {
        QUrl result;
        result.setScheme(scheme);
        result.setHost(host);
        result.setPort(port);
        result.setPath(path);
        return result;
    }
};

struct EndpointPolicy
{
    QStringList allowedSchemes { QStringLiteral("https") };
    QStringList allowedHostSuffixes;
    quint16 minimumPort = 1;
    quint16 maximumPort = 65535;
    int minimumRemainingLifetimeMsecs = 1000;

    bool accepts(const LoginEndpoint &endpoint, QString *reason = nullptr) const;
};

struct ClientConfig
{
    QString gatewayHost;
    quint16 gatewayPort = 0;
    int responseTimeoutMsecs = 3000;
    int maximumAttempts = 3;
    qint64 maximumResponseBytes = 16 * 1024;
    EndpointPolicy endpointPolicy;

    bool isValid(QString *reason = nullptr) const;
};

enum class DecodeDisposition
{
    Accepted,
    Ignore,
    Rejected
};

struct DecodeResult
{
    DecodeDisposition disposition = DecodeDisposition::Rejected;
    LoginEndpoint endpoint;
    QString errorMessage;

    static DecodeResult accepted(LoginEndpoint value)
    {
        DecodeResult result;
        result.disposition = DecodeDisposition::Accepted;
        result.endpoint = std::move(value);
        return result;
    }

    static DecodeResult ignored()
    {
        DecodeResult result;
        result.disposition = DecodeDisposition::Ignore;
        return result;
    }

    static DecodeResult rejected(QString message)
    {
        DecodeResult result;
        result.disposition = DecodeDisposition::Rejected;
        result.errorMessage = std::move(message);
        return result;
    }
};

} // namespace spa

Q_DECLARE_METATYPE(spa::LoginEndpoint)

#endif // SPA_TYPES_H
