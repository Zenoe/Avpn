#include "authentication/authenticationController.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>
#include <QStringList>

#include <utility>

#include "passwordCrypto.h"

namespace {

QString responseMessage(const QJsonObject &response, const QString &fallback)
{
    QString message = response.value(QLatin1String("msg")).toString();
    const QString errorCode = response.value(QLatin1String("errorCode")).toString();
    if (message.isEmpty()) {
        message = fallback;
    }
    if (!errorCode.isEmpty()) {
        message += QStringLiteral("（%1）").arg(errorCode);
    }
    return message;
}

bool isSensitiveField(const QString &field)
{
    static const QStringList sensitiveFields {
        QStringLiteral("password"),
        QStringLiteral("encryptedpassword"),
        QStringLiteral("username"),
        QStringLiteral("code"),
        QStringLiteral("uuid"),
        QStringLiteral("img"),
        QStringLiteral("token"),
        QStringLiteral("access_token"),
        QStringLiteral("refresh_token"),
        QStringLiteral("spaticket"),
        QStringLiteral("ticket"),
        QStringLiteral("authorization"),
        QStringLiteral("credentialid"),
        QStringLiteral("authproof"),
        QStringLiteral("challenge_id"),
        QStringLiteral("challenge_url"),
        QStringLiteral("deviceid"),
        QStringLiteral("nonce")
    };
    const QString normalized = field.toLower();
    return sensitiveFields.contains(normalized)
            || normalized.contains(QStringLiteral("password"))
            || normalized.contains(QStringLiteral("token"))
            || normalized.contains(QStringLiteral("ticket"))
            || normalized.contains(QStringLiteral("authorization"));
}

QJsonValue redactJsonValue(const QJsonValue &value);

QJsonObject redactJsonObject(const QJsonObject &object)
{
    QJsonObject redacted;
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        redacted.insert(it.key(), isSensitiveField(it.key())
                                      ? QJsonValue(QStringLiteral("<redacted>"))
                                      : redactJsonValue(it.value()));
    }
    return redacted;
}

QJsonValue redactJsonValue(const QJsonValue &value)
{
    if (value.isObject()) {
        return redactJsonObject(value.toObject());
    }
    if (value.isArray()) {
        QJsonArray redacted;
        for (const QJsonValue &item : value.toArray()) {
            redacted.append(redactJsonValue(item));
        }
        return redacted;
    }
    return value;
}

QString redactedBody(const QByteArray &body)
{
    if (body.isEmpty()) {
        return QStringLiteral("<empty>");
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return QStringLiteral("<non-JSON; %1 bytes; parse error: %2>")
                .arg(body.size()).arg(parseError.errorString());
    }
    if (document.isObject()) {
        return QString::fromUtf8(QJsonDocument(redactJsonObject(document.object()))
                                         .toJson(QJsonDocument::Compact));
    }
    if (document.isArray()) {
        return QString::fromUtf8(QJsonDocument(redactJsonValue(document.array()).toArray())
                                         .toJson(QJsonDocument::Compact));
    }
    return QStringLiteral("<empty JSON document>");
}

QString headerSummary(const QNetworkRequest &request)
{
    QStringList headers;
    for (const QByteArray &name : request.rawHeaderList()) {
        const QByteArray value = request.rawHeader(name);
        const QString field = QString::fromLatin1(name);
        headers.append(QStringLiteral("%1=%2").arg(
                field,
                isSensitiveField(field) ? QStringLiteral("<redacted; %1 bytes>").arg(value.size())
                                        : QString::fromLatin1(value)));
    }
    return headers.join(QStringLiteral(", "));
}

void logRequest(const QString &operation, const QNetworkRequest &request, const QByteArray &body = {})
{
    qInfo().noquote() << QStringLiteral("[AUTH][REQUEST] operation=%1 url=%2 headers={%3} body=%4")
                              .arg(operation, request.url().toString(QUrl::FullyEncoded),
                                   headerSummary(request), redactedBody(body));
}

void logResponse(const QString &operation, const QUrl &url, int httpStatus, const QByteArray &body)
{
    qInfo().noquote() << QStringLiteral("[AUTH][RESPONSE] operation=%1 url=%2 http=%3 body=%4")
                              .arg(operation, url.toString(QUrl::FullyEncoded))
                              .arg(httpStatus)
                              .arg(redactedBody(body));
}

QString sslErrorName(QSslError::SslError error)
{
    switch (error) {
    case QSslError::SelfSignedCertificate:
        return QStringLiteral("SelfSignedCertificate");
    case QSslError::SelfSignedCertificateInChain:
        return QStringLiteral("SelfSignedCertificateInChain");
    case QSslError::UnableToGetIssuerCertificate:
        return QStringLiteral("UnableToGetIssuerCertificate");
    case QSslError::UnableToGetLocalIssuerCertificate:
        return QStringLiteral("UnableToGetLocalIssuerCertificate");
    case QSslError::UnableToVerifyFirstCertificate:
        return QStringLiteral("UnableToVerifyFirstCertificate");
    case QSslError::CertificateUntrusted:
        return QStringLiteral("CertificateUntrusted");
    case QSslError::HostNameMismatch:
        return QStringLiteral("HostNameMismatch");
    case QSslError::CertificateNotYetValid:
        return QStringLiteral("CertificateNotYetValid");
    case QSslError::CertificateExpired:
        return QStringLiteral("CertificateExpired");
    case QSslError::CertificateRevoked:
        return QStringLiteral("CertificateRevoked");
    case QSslError::CertificateBlacklisted:
        return QStringLiteral("CertificateBlacklisted");
    case QSslError::NoPeerCertificate:
        return QStringLiteral("NoPeerCertificate");
    default:
        return QStringLiteral("SslError(%1)").arg(static_cast<int>(error));
    }
}

bool isAllowedPrivateGatewayError(QSslError::SslError error)
{
    return error == QSslError::HostNameMismatch
            || error == QSslError::SelfSignedCertificate
            || error == QSslError::SelfSignedCertificateInChain;
}

} // namespace

namespace authentication {

Controller::Controller(QObject *parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this))
{
}

bool Controller::active() const
{
    return m_active;
}

bool Controller::busy() const
{
    return m_busy;
}

bool Controller::authenticated() const
{
    return m_authenticated;
}

QVariantList Controller::providers() const
{
    return m_providers;
}

QString Controller::captchaImageDataUrl() const
{
    return m_captchaImageDataUrl;
}

QString Controller::captchaUuid() const
{
    return m_captchaUuid;
}

bool Controller::captchaRequired() const
{
    return m_captchaRequired;
}

QString Controller::statusMessage() const
{
    return m_statusMessage;
}

QString Controller::statusType() const
{
    return m_statusType;
}

void Controller::configureGateway(const QUrl &gatewayUrl, const QByteArray &spaTicket)
{
    if (!gatewayUrl.isValid() || gatewayUrl.scheme() != QLatin1String("https") || gatewayUrl.host().isEmpty()) {
        setStatus(tr("SPA 返回的登录网关地址无效"), QStringLiteral("error"));
        qWarning().noquote() << "[AUTH] rejected invalid SPA gateway endpoint";
        return;
    }
    m_gatewayUrl = gatewayUrl;
    m_spaTicket = spaTicket;
    m_spaTicketPending = !m_spaTicket.isEmpty();
    m_accessToken.fill('\0');
    m_accessToken.clear();
    m_providers.clear();
    emit providersChanged();
    clearCaptcha();
    setAuthenticated(false);
    if (!m_active) {
        m_active = true;
        emit activeChanged();
    }
    reloadProviders();
}

void Controller::reloadProviders()
{
    if (!m_active || m_gatewayUrl.host().isEmpty()) {
        setStatus(tr("尚未获得登录网关地址"), QStringLiteral("error"));
        return;
    }
    if (m_busy) {
        return;
    }

    setStatus(tr("正在获取认证方式…"), QStringLiteral("info"));
    setBusy(true);
    const QNetworkRequest request = createRequest(QStringLiteral("/auth/external/providers"));
    logRequest(QStringLiteral("providers"), request);
    QNetworkReply *reply = m_networkManager->get(request);
    handleReply(reply, QStringLiteral("providers"), [this](const QJsonObject &response) {
        const QJsonArray items = response.value(QLatin1String("data")).toObject()
                                     .value(QLatin1String("items")).toArray();
        QVariantList providers;
        for (const QJsonValue &itemValue : items) {
            const QJsonObject item = itemValue.toObject();
            const QString type = item.value(QLatin1String("type")).toString();
            if (type.isEmpty()) {
                continue;
            }
            QVariantMap provider;
            provider.insert(QStringLiteral("id"), item.value(QLatin1String("id")).toInt());
            provider.insert(QStringLiteral("name"), item.value(QLatin1String("name")).toString(type));
            provider.insert(QStringLiteral("type"), type);
            provider.insert(QStringLiteral("mode"), item.value(QLatin1String("mode")).toString());
            provider.insert(QStringLiteral("default"), item.value(QLatin1String("default")).toBool());
            provider.insert(QStringLiteral("captchaEnabled"), item.value(QLatin1String("captchaEnabled")).toBool());
            providers.append(provider);
        }

        m_providers = providers;
        emit providersChanged();
        if (m_providers.isEmpty()) {
            setStatus(tr("网关未提供可用的认证方式"), QStringLiteral("error"));
            qWarning().noquote() << "[AUTH] gateway returned no available providers";
            return;
        }
        setStatus(tr("请选择认证方式"), QStringLiteral("info"));

        for (const QVariant &providerValue : std::as_const(m_providers)) {
            const QVariantMap provider = providerValue.toMap();
            if (provider.value(QStringLiteral("type")).toString() == QLatin1String("LOCAL")
                && provider.value(QStringLiteral("default")).toBool()) {
                loadCaptcha(provider.value(QStringLiteral("id")).toInt());
                break;
            }
        }
    });
}

void Controller::loadCaptcha(int providerId)
{
    if (m_busy || providerId <= 0) {
        return;
    }

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("providerId"), QString::number(providerId));
    setBusy(true);
    const QNetworkRequest request = createRequest(QStringLiteral("/auth/captcha"), query);
    logRequest(QStringLiteral("captcha"), request);
    QNetworkReply *reply = m_networkManager->get(request);
    handleReply(reply, QStringLiteral("captcha"), [this](const QJsonObject &response) {
        const QJsonObject data = response.value(QLatin1String("data")).toObject();
        m_captchaRequired = data.value(QLatin1String("captchaEnabled")).toBool();
        m_captchaUuid = data.value(QLatin1String("uuid")).toString();
        const QString image = data.value(QLatin1String("img")).toString();
        m_captchaImageDataUrl = image.isEmpty() ? QString()
                                                : QStringLiteral("data:image/jpeg;base64,") + image;
        if (m_captchaRequired && (m_captchaUuid.isEmpty() || m_captchaImageDataUrl.isEmpty())) {
            clearCaptcha();
            setStatus(tr("验证码服务返回的数据不完整"), QStringLiteral("error"));
            qWarning().noquote() << "[AUTH] captcha response was incomplete";
            return;
        }
        emit captchaChanged();
    });
}

void Controller::login(const QString &username, const QString &password,
                       const QString &captchaCode, int providerId)
{
    if (m_busy) {
        return;
    }
    if (username.trimmed().isEmpty()) {
        setStatus(tr("请输入工号或用户名"), QStringLiteral("error"));
        return;
    }
    if (password.isEmpty()) {
        setStatus(tr("请输入密码"), QStringLiteral("error"));
        return;
    }
    if (providerId <= 0) {
        setStatus(tr("未选择本地账号认证方式"), QStringLiteral("error"));
        return;
    }
    if (m_captchaRequired && (captchaCode.trimmed().isEmpty() || m_captchaUuid.isEmpty())) {
        setStatus(tr("请输入验证码"), QStringLiteral("error"));
        return;
    }

    QString encryptedPassword;
    QString encryptionError;
    if (!crypto::encryptPassword(password, &encryptedPassword, &encryptionError)) {
        setStatus(encryptionError, QStringLiteral("error"));
        qWarning().noquote() << "[AUTH] password encryption failed";
        return;
    }

    QJsonObject payload {
        { QStringLiteral("username"), username.trimmed() },
        { QStringLiteral("encryptedPassword"), encryptedPassword },
        { QStringLiteral("authProviderId"), providerId }
    };
    encryptedPassword.fill(QChar('\0'));
    if (m_captchaRequired) {
        payload.insert(QStringLiteral("uuid"), m_captchaUuid);
        payload.insert(QStringLiteral("code"), captchaCode.trimmed());
    }

    setStatus(tr("正在登录…"), QStringLiteral("info"));
    setBusy(true);
    QNetworkRequest request = createRequest(QStringLiteral("/auth/login"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QByteArray body = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    logRequest(QStringLiteral("local-login"), request, body);
    QNetworkReply *reply = m_networkManager->post(request, body);
    body.fill('\0');
    payload = {};
    handleReply(reply, QStringLiteral("local-login"), [this](const QJsonObject &response) {
        const QJsonObject data = response.value(QLatin1String("data")).toObject();
        const QString accessToken = data.value(QLatin1String("access_token")).toString();
        if (accessToken.isEmpty()) {
            setStatus(tr("登录响应中缺少访问令牌"), QStringLiteral("error"));
            qWarning().noquote() << "[AUTH] login response did not contain access token";
            return;
        }
        m_accessToken = accessToken.toUtf8();
        if (data.value(QLatin1String("mfa_required")).toBool()) {
            setAuthenticated(false);
            setStatus(tr("账号密码验证成功，请完成二次认证"), QStringLiteral("warning"));
            qWarning().noquote() << "[AUTH] MFA is required; authenticated state remains pending";
            return;
        }
        setAuthenticated(true);
        setStatus(tr("登录成功"), QStringLiteral("success"));
        emit loginSucceeded();
    });
}

void Controller::showUnsupportedProvider(const QString &providerName)
{
    const QString name = providerName.isEmpty() ? tr("该认证方式") : providerName;
    setStatus(tr("%1暂未提供客户端所需的认证接口").arg(name), QStringLiteral("warning"));
    qWarning().noquote() << "[AUTH] provider is advertised but its client flow is undocumented:" << name;
}

QUrl Controller::apiUrl(const QString &path, const QUrlQuery &query) const
{
    QUrl result = m_gatewayUrl;
    result.setPath(QStringLiteral("/api") + path);
    result.setQuery(query);
    return result;
}

QNetworkRequest Controller::createRequest(const QString &path, const QUrlQuery &query)
{
    QNetworkRequest request(apiUrl(path, query));
    request.setRawHeader("Accept", "application/json");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::SameOriginRedirectPolicy);
    request.setTransferTimeout(10000);
    if (m_spaTicketPending) {
        request.setRawHeader("X-SPA-Ticket", m_spaTicket);
    }
    return request;
}

void Controller::setBusy(bool value)
{
    if (m_busy == value) {
        return;
    }
    m_busy = value;
    emit busyChanged();
}

void Controller::setAuthenticated(bool value)
{
    if (m_authenticated == value) {
        return;
    }
    m_authenticated = value;
    emit authenticatedChanged();
}

void Controller::setStatus(const QString &message, const QString &type)
{
    if (m_statusMessage == message && m_statusType == type) {
        return;
    }
    m_statusMessage = message;
    m_statusType = type;
    emit statusChanged();
}

void Controller::clearCaptcha()
{
    m_captchaImageDataUrl.clear();
    m_captchaUuid.clear();
    m_captchaRequired = false;
    emit captchaChanged();
}

void Controller::handleReply(QNetworkReply *reply, const QString &operation,
                             const std::function<void(const QJsonObject &)> &onSuccess)
{
    m_reply = reply;
    connect(reply, &QNetworkReply::sslErrors, this, [this, operation, reply](const QList<QSslError> &errors) {
        QStringList messages;
        for (const QSslError &error : errors) {
            messages.append(sslErrorName(error.error()));
        }
        qWarning().noquote() << QStringLiteral("[AUTH][SSL] operation=%1 url=%2 errors=%3")
                                      .arg(operation, reply->url().toString(QUrl::FullyEncoded),
                                           messages.join(QStringLiteral("; ")));

        bool hostNameMismatch = false;
        bool selfSigned = false;
        bool onlyAllowedPrivateGatewayErrors = !errors.isEmpty();
        for (const QSslError &error : errors) {
            hostNameMismatch = hostNameMismatch || error.error() == QSslError::HostNameMismatch;
            selfSigned = selfSigned || error.error() == QSslError::SelfSignedCertificate
                    || error.error() == QSslError::SelfSignedCertificateInChain;
            onlyAllowedPrivateGatewayErrors = onlyAllowedPrivateGatewayErrors
                    && isAllowedPrivateGatewayError(error.error());
        }

        if (selfSigned && onlyAllowedPrivateGatewayErrors
            && reply->url().scheme() == QLatin1String("https")
            && reply->url().host().compare(m_gatewayUrl.host(), Qt::CaseInsensitive) == 0
            && reply->url().port(443) == m_gatewayUrl.port(443)) {
            qWarning().noquote()
                    << QStringLiteral("[AUTH][TLS] accepted private self-signed gateway certificate operation=%1 url=%2 errors=%3")
                               .arg(operation, reply->url().toString(QUrl::FullyEncoded),
                                    messages.join(QStringLiteral("; ")));
            reply->ignoreSslErrors(errors);
            return;
        }

        const QString reason = hostNameMismatch && selfSigned
                ? tr("登录网关证书为自签名证书，且证书不包含当前访问地址")
                : (hostNameMismatch ? tr("登录网关证书不包含当前访问地址")
                                    : tr("登录网关证书不受信任"));
        reply->setProperty("authenticationTlsError", reason);
        qWarning().noquote() << QStringLiteral("[AUTH][TLS] certificate rejected operation=%1 url=%2 policy=strict")
                                      .arg(operation, reply->url().toString(QUrl::FullyEncoded));
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, operation, onSuccess]() {
        if (reply != m_reply) {
            reply->deleteLater();
            return;
        }
        m_reply = nullptr;
        setBusy(false);

        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (httpStatus > 0 && m_spaTicketPending) {
            m_spaTicketPending = false;
            m_spaTicket.fill('\0');
            m_spaTicket.clear();
        }
        const QByteArray body = reply->readAll();
        logResponse(operation, reply->url(), httpStatus, body);
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
        if (reply->error() != QNetworkReply::NoError) {
            const QString tlsReason = reply->property("authenticationTlsError").toString();
            const QString reason = !tlsReason.isEmpty()
                    ? tlsReason
                    : (document.isObject()
                           ? responseMessage(document.object(), reply->errorString())
                           : reply->errorString());
            setStatus(tr("请求失败：%1").arg(reason), QStringLiteral("error"));
            logFailure(operation, reply->url(), httpStatus, reason);
            reply->deleteLater();
            return;
        }
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            const QString reason = tr("网关返回了无效的 JSON 响应");
            setStatus(reason, QStringLiteral("error"));
            logFailure(operation, reply->url(), httpStatus, reason);
            reply->deleteLater();
            return;
        }

        const QJsonObject response = document.object();
        if (httpStatus < 200 || httpStatus >= 300 || response.value(QLatin1String("code")).toInt(-1) != 200) {
            const QString reason = responseMessage(response, tr("网关拒绝了请求"));
            setStatus(tr("请求失败：%1").arg(reason), QStringLiteral("error"));
            logFailure(operation, reply->url(), httpStatus, reason);
            reply->deleteLater();
            return;
        }

        qInfo().noquote() << QStringLiteral("[AUTH][SUCCESS] operation=%1 url=%2 http=%3")
                                  .arg(operation, reply->url().toString(QUrl::FullyEncoded))
                                  .arg(httpStatus);
        onSuccess(response);
        reply->deleteLater();
    });
}

void Controller::logFailure(const QString &operation, const QUrl &url, int httpStatus,
                            const QString &reason) const
{
    qWarning().noquote() << QStringLiteral("[AUTH][FAILED] operation=%1 url=%2 http=%3 reason=%4")
                                  .arg(operation, url.toString(QUrl::FullyEncoded))
                                  .arg(httpStatus)
                                  .arg(reason);
}

} // namespace authentication
