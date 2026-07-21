#include "spa/spaUiController.h"

#include <QUrl>

#include "spa/gmSpaCodec.h"
#include "spa/spaDeviceIdentity.h"

namespace spa {

UiController::UiController(const QString &appVersion, QObject *parent)
    : QObject(parent)
{
    if (!GmSpaCodec::selfTest(&m_initializationError)) {
        setStatus(m_initializationError, QStringLiteral("error"));
        return;
    }

    m_defaultConfig = ConfigLoader::loadDefault(&m_initializationError);
    if (!m_initializationError.isEmpty()) {
        setStatus(m_initializationError, QStringLiteral("error"));
        return;
    }

    QString identityError;
    const DeviceIdentity identity = DeviceIdentityProvider::current(&identityError);
    if (!identity.isValid()) {
        m_initializationError = identityError;
        setStatus(m_initializationError, QStringLiteral("error"));
        return;
    }

    m_codec = std::make_shared<GmSpaCodec>(m_defaultConfig.protocol, identity, appVersion);
    m_client = std::make_unique<Client>(m_codec);

    connect(m_client.get(), &Client::attemptStarted, this, [this](int attempt, int maximumAttempts) {
        setStatus(tr("正在发送 SPA 请求（%1/%2）…").arg(attempt).arg(maximumAttempts), QStringLiteral("info"));
    });
    connect(m_client.get(), &Client::endpointReady, this, [this](const LoginEndpoint &endpoint) {
        m_loginEndpoint = endpoint;
        setBusy(false);
        setSucceeded(true);
        setStatus(tr("SPA 验证成功，登录服务：%1").arg(endpoint.url().toString()), QStringLiteral("success"));
        emit spaSucceeded();
    });
    connect(m_client.get(), &Client::failed, this, [this](Client::Error, const QString &message) {
        setBusy(false);
        setSucceeded(false);
        setStatus(tr("SPA 请求失败：%1").arg(message), QStringLiteral("error"));
    });
}

bool UiController::busy() const
{
    return m_busy;
}

bool UiController::succeeded() const
{
    return m_succeeded;
}

QString UiController::statusMessage() const
{
    return m_statusMessage;
}

QString UiController::statusType() const
{
    return m_statusType;
}

LoginEndpoint UiController::loginEndpoint() const
{
    return m_loginEndpoint;
}

void UiController::start(const QString &gatewayHost)
{
    if (m_busy) {
        return;
    }
    if (!m_initializationError.isEmpty() || !m_client) {
        setStatus(m_initializationError.isEmpty() ? tr("SPA 模块初始化失败") : m_initializationError,
                  QStringLiteral("error"));
        return;
    }

    QString hostError;
    const QString host = normalizedHost(gatewayHost, &hostError);
    if (host.isEmpty()) {
        setStatus(hostError, QStringLiteral("error"));
        return;
    }

    m_loginEndpoint = {};
    setSucceeded(false);
    setBusy(true);
    setStatus(tr("正在连接零信任网关 %1:%2…").arg(host).arg(m_defaultConfig.gatewayPort),
              QStringLiteral("info"));

    ClientConfig config = m_defaultConfig.client;
    config.gatewayHost = host;
    m_client->setConfig(config);
    m_client->discover();
}

void UiController::cancel()
{
    if (m_client) {
        m_client->cancel();
    }
    setBusy(false);
    setStatus(tr("SPA 请求已取消"), QStringLiteral("info"));
}

QString UiController::normalizedHost(const QString &input, QString *errorMessage)
{
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty()) {
        if (errorMessage) {
            *errorMessage = tr("请输入零信任地址");
        }
        return {};
    }

    const bool hasScheme = trimmed.contains(QLatin1String("://"));
    const QUrl url(hasScheme ? trimmed : QStringLiteral("udp://") + trimmed);
    if (!url.isValid() || url.host().isEmpty() || !url.userInfo().isEmpty()
        || (!url.path().isEmpty() && url.path() != QLatin1String("/"))
        || url.hasQuery() || url.hasFragment()) {
        if (errorMessage) {
            *errorMessage = tr("零信任地址格式无效，请输入域名或 IP 地址");
        }
        return {};
    }
    if (url.port(-1) != -1 && url.port() != 16888) {
        if (errorMessage) {
            *errorMessage = tr("SPA UDP 端口固定为 16888，请只输入地址");
        }
        return {};
    }
    return url.host();
}

void UiController::setBusy(bool value)
{
    if (m_busy == value) {
        return;
    }
    m_busy = value;
    emit busyChanged();
}

void UiController::setSucceeded(bool value)
{
    if (m_succeeded == value) {
        return;
    }
    m_succeeded = value;
    emit succeededChanged();
}

void UiController::setStatus(const QString &message, const QString &type)
{
    if (m_statusMessage != message) {
        m_statusMessage = message;
        emit statusMessageChanged();
    }
    if (m_statusType != type) {
        m_statusType = type;
        emit statusTypeChanged();
    }
}

} // namespace spa
