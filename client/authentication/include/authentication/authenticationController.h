#ifndef AUTHENTICATION_CONTROLLER_H
#define AUTHENTICATION_CONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QVariantList>

#include <functional>

class QNetworkAccessManager;
class QNetworkReply;

namespace authentication {

class Controller final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool authenticated READ authenticated NOTIFY authenticatedChanged)
    Q_PROPERTY(QVariantList providers READ providers NOTIFY providersChanged)
    Q_PROPERTY(QString captchaImageDataUrl READ captchaImageDataUrl NOTIFY captchaChanged)
    Q_PROPERTY(QString captchaUuid READ captchaUuid NOTIFY captchaChanged)
    Q_PROPERTY(bool captchaRequired READ captchaRequired NOTIFY captchaChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(QString statusType READ statusType NOTIFY statusChanged)

public:
    explicit Controller(QObject *parent = nullptr);

    bool active() const;
    bool busy() const;
    bool authenticated() const;
    QVariantList providers() const;
    QString captchaImageDataUrl() const;
    QString captchaUuid() const;
    bool captchaRequired() const;
    QString statusMessage() const;
    QString statusType() const;

    void configureGateway(const QUrl &gatewayUrl, const QByteArray &spaTicket);

    Q_INVOKABLE void reloadProviders();
    Q_INVOKABLE void loadCaptcha(int providerId);
    Q_INVOKABLE void login(const QString &username, const QString &password,
                           const QString &captchaCode, int providerId);
    Q_INVOKABLE void showUnsupportedProvider(const QString &providerName);

signals:
    void activeChanged();
    void busyChanged();
    void authenticatedChanged();
    void providersChanged();
    void captchaChanged();
    void statusChanged();
    void loginSucceeded();

private:
    QUrl apiUrl(const QString &path, const QUrlQuery &query = {}) const;
    QNetworkRequest createRequest(const QString &path, const QUrlQuery &query = {});
    void setBusy(bool value);
    void setAuthenticated(bool value);
    void setStatus(const QString &message, const QString &type);
    void clearCaptcha();
    void handleReply(QNetworkReply *reply, const QString &operation,
                     const std::function<void(const QJsonObject &)> &onSuccess);
    void logFailure(const QString &operation, const QUrl &url, int httpStatus,
                    const QString &reason) const;

    QNetworkAccessManager *m_networkManager = nullptr;
    QNetworkReply *m_reply = nullptr;
    QUrl m_gatewayUrl;
    QByteArray m_spaTicket;
    bool m_spaTicketPending = false;
    bool m_active = false;
    bool m_busy = false;
    bool m_authenticated = false;
    QVariantList m_providers;
    QString m_captchaImageDataUrl;
    QString m_captchaUuid;
    bool m_captchaRequired = false;
    QString m_statusMessage;
    QString m_statusType = QStringLiteral("info");
    QByteArray m_accessToken;
};

} // namespace authentication

#endif // AUTHENTICATION_CONTROLLER_H
