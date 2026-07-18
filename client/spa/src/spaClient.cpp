#include "spa/spaClient.h"

#include <QUuid>

#include <utility>

namespace {

void setReason(QString *reason, const QString &message)
{
    if (reason) {
        *reason = message;
    }
}

bool hostMatchesSuffix(const QString &host, const QString &configuredSuffix)
{
    QString suffix = configuredSuffix.trimmed().toLower();
    while (suffix.startsWith(QLatin1Char('.'))) {
        suffix.remove(0, 1);
    }

    if (suffix.isEmpty()) {
        return false;
    }

    const QString normalizedHost = host.trimmed().toLower();
    return normalizedHost == suffix || normalizedHost.endsWith(QLatin1Char('.') + suffix);
}

} // namespace

namespace spa {

bool EndpointPolicy::accepts(const LoginEndpoint &endpoint, QString *reason) const
{
    if (!endpoint.isStructurallyValid()) {
        setReason(reason, QStringLiteral("The SPA response contains an incomplete login endpoint"));
        return false;
    }

    bool schemeAllowed = false;
    for (const QString &allowedScheme : allowedSchemes) {
        if (endpoint.scheme.compare(allowedScheme, Qt::CaseInsensitive) == 0) {
            schemeAllowed = true;
            break;
        }
    }
    if (!schemeAllowed) {
        setReason(reason, QStringLiteral("The login endpoint scheme is not allowed"));
        return false;
    }

    if (minimumPort == 0 || maximumPort < minimumPort || endpoint.port < minimumPort
        || endpoint.port > maximumPort) {
        setReason(reason, QStringLiteral("The login endpoint port is outside the allowed range"));
        return false;
    }

    if (!allowedHostSuffixes.isEmpty()) {
        bool hostAllowed = false;
        for (const QString &suffix : allowedHostSuffixes) {
            if (hostMatchesSuffix(endpoint.host, suffix)) {
                hostAllowed = true;
                break;
            }
        }
        if (!hostAllowed) {
            setReason(reason, QStringLiteral("The login endpoint host is not allowed"));
            return false;
        }
    }

    const QDateTime minimumExpiry = QDateTime::currentDateTimeUtc().addMSecs(minimumRemainingLifetimeMsecs);
    if (endpoint.expiresAtUtc <= minimumExpiry) {
        setReason(reason, QStringLiteral("The login endpoint expires too soon"));
        return false;
    }

    return true;
}

bool ClientConfig::isValid(QString *reason) const
{
    if (gatewayHost.trimmed().isEmpty()) {
        setReason(reason, QStringLiteral("The SPA gateway host is empty"));
        return false;
    }
    if (gatewayPort == 0) {
        setReason(reason, QStringLiteral("The SPA gateway port is invalid"));
        return false;
    }
    if (responseTimeoutMsecs <= 0) {
        setReason(reason, QStringLiteral("The SPA response timeout must be positive"));
        return false;
    }
    if (maximumAttempts <= 0) {
        setReason(reason, QStringLiteral("The SPA maximum attempt count must be positive"));
        return false;
    }
    if (maximumResponseBytes <= 0) {
        setReason(reason, QStringLiteral("The SPA maximum response size must be positive"));
        return false;
    }
    if (endpointPolicy.minimumPort == 0 || endpointPolicy.maximumPort < endpointPolicy.minimumPort) {
        setReason(reason, QStringLiteral("The login endpoint port policy is invalid"));
        return false;
    }
    return true;
}

Client::Client(std::shared_ptr<Codec> codec, QObject *parent)
    : QObject(parent), m_codec(std::move(codec))
{
    qRegisterMetaType<spa::LoginEndpoint>();

    m_responseTimer.setSingleShot(true);
    connect(&m_responseTimer, &QTimer::timeout, this, &Client::handleResponseTimeout);
    connect(&m_socket, &QUdpSocket::connected, this, &Client::handleConnected);
    connect(&m_socket, &QUdpSocket::readyRead, this, &Client::handleReadyRead);
    connect(&m_socket, &QUdpSocket::errorOccurred, this, &Client::handleSocketError);
}

Client::State Client::state() const
{
    return m_state;
}

ClientConfig Client::config() const
{
    return m_config;
}

void Client::setConfig(const ClientConfig &config)
{
    if (m_active) {
        return;
    }
    m_config = config;
}

void Client::discover(const QVariantMap &attributes)
{
    cancel();
    setState(State::Idle);

    QString configurationError;
    if (!m_codec) {
        finishWithError(Error::InvalidConfiguration, QStringLiteral("No SPA codec is configured"));
        return;
    }
    if (!m_config.isValid(&configurationError)) {
        finishWithError(Error::InvalidConfiguration, configurationError);
        return;
    }

    m_request.requestId = createRequestId();
    m_request.createdAtUtc = QDateTime::currentDateTimeUtc();
    m_request.attributes = attributes;

    const EncodeResult encoded = m_codec->encodeRequest(m_request);
    if (!encoded.isSuccess()) {
        finishWithError(Error::EncodingFailed,
                        encoded.errorMessage.isEmpty() ? QStringLiteral("The SPA codec produced an empty request")
                                                       : encoded.errorMessage);
        return;
    }

    m_requestDatagram = encoded.datagram;
    m_attempt = 0;
    m_active = true;
    setState(State::Connecting);

    m_socket.connectToHost(m_config.gatewayHost, m_config.gatewayPort, QIODevice::ReadWrite);
    if (m_socket.state() == QAbstractSocket::ConnectedState && m_state == State::Connecting) {
        sendAttempt();
    }
}

void Client::cancel()
{
    if (!m_active) {
        return;
    }

    m_active = false;
    m_responseTimer.stop();
    resetTransport();
    clearOperationData();
    setState(State::Cancelled);
}

void Client::handleConnected()
{
    if (m_active && m_state == State::Connecting) {
        sendAttempt();
    }
}

void Client::handleReadyRead()
{
    while (m_active && m_socket.hasPendingDatagrams()) {
        const qint64 pendingSize = m_socket.pendingDatagramSize();
        if (pendingSize < 0) {
            finishWithError(Error::NetworkError, QStringLiteral("Failed to inspect the SPA response"));
            return;
        }
        if (pendingSize > m_config.maximumResponseBytes) {
            m_socket.readDatagram(nullptr, 0);
            finishWithError(Error::ResponseTooLarge, QStringLiteral("The SPA response exceeds the configured size limit"));
            return;
        }

        QByteArray datagram(pendingSize, Qt::Uninitialized);
        const qint64 bytesRead = m_socket.readDatagram(datagram.data(), datagram.size());
        if (bytesRead < 0) {
            finishWithError(Error::NetworkError, QStringLiteral("Failed to read the SPA response"));
            return;
        }
        datagram.resize(bytesRead);

        const DecodeResult decoded = m_codec->decodeResponse(datagram, m_request);
        if (decoded.disposition == DecodeDisposition::Ignore) {
            continue;
        }
        if (decoded.disposition == DecodeDisposition::Rejected) {
            finishWithError(Error::ResponseRejected,
                            decoded.errorMessage.isEmpty() ? QStringLiteral("The SPA response was rejected by the codec")
                                                           : decoded.errorMessage);
            return;
        }

        QString policyError;
        if (!m_config.endpointPolicy.accepts(decoded.endpoint, &policyError)) {
            finishWithError(Error::EndpointRejected, policyError);
            return;
        }

        const LoginEndpoint endpoint = decoded.endpoint;
        m_active = false;
        m_responseTimer.stop();
        resetTransport();
        clearOperationData();
        setState(State::Succeeded);
        emit endpointReady(endpoint);
        return;
    }
}

void Client::handleSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    if (m_active) {
        finishWithError(Error::NetworkError, m_socket.errorString());
    }
}

void Client::handleResponseTimeout()
{
    if (!m_active) {
        return;
    }

    if (m_attempt >= m_config.maximumAttempts) {
        finishWithError(Error::Timeout, QStringLiteral("The SPA gateway did not respond before the retry limit"));
        return;
    }

    sendAttempt();
}

void Client::sendAttempt()
{
    if (!m_active || m_socket.state() != QAbstractSocket::ConnectedState) {
        return;
    }

    ++m_attempt;
    emit attemptStarted(m_attempt, m_config.maximumAttempts);

    const qint64 bytesWritten = m_socket.write(m_requestDatagram);
    if (bytesWritten != m_requestDatagram.size()) {
        finishWithError(Error::NetworkError, QStringLiteral("Failed to send the complete SPA request"));
        return;
    }

    setState(State::WaitingForResponse);
    m_responseTimer.start(m_config.responseTimeoutMsecs);
}

void Client::setState(State state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged(m_state);
}

void Client::finishWithError(Error error, const QString &message)
{
    m_active = false;
    m_responseTimer.stop();
    resetTransport();
    clearOperationData();
    setState(State::Failed);
    emit failed(error, message);
}

void Client::resetTransport()
{
    m_socket.abort();
}

void Client::clearOperationData()
{
    m_request.requestId.fill('\0');
    m_request = {};
    m_requestDatagram.fill('\0');
    m_requestDatagram.clear();
    m_attempt = 0;
}

QByteArray Client::createRequestId()
{
    return QUuid::createUuid().toRfc4122() + QUuid::createUuid().toRfc4122();
}

} // namespace spa
