#ifndef SPA_CLIENT_H
#define SPA_CLIENT_H

#include <QAbstractSocket>
#include <QObject>
#include <QTimer>
#include <QUdpSocket>

#include <memory>

#include "spa/spaCodec.h"
#include "spa/spaTypes.h"

namespace spa {

class Client final : public QObject
{
    Q_OBJECT

public:
    enum class State
    {
        Idle,
        Connecting,
        WaitingForResponse,
        Succeeded,
        Failed,
        Cancelled
    };
    Q_ENUM(State)

    enum class Error
    {
        InvalidConfiguration,
        EncodingFailed,
        NetworkError,
        Timeout,
        ResponseTooLarge,
        ResponseRejected,
        EndpointRejected
    };
    Q_ENUM(Error)

    explicit Client(std::shared_ptr<Codec> codec, QObject *parent = nullptr);

    State state() const;
    ClientConfig config() const;
    void setConfig(const ClientConfig &config);

public slots:
    void discover(const QVariantMap &attributes = {});
    void cancel();

signals:
    void stateChanged(spa::Client::State state);
    void attemptStarted(int attempt, int maximumAttempts);
    void endpointReady(const spa::LoginEndpoint &endpoint);
    void failed(spa::Client::Error error, const QString &message);

private slots:
    void handleConnected();
    void handleReadyRead();
    void handleSocketError(QAbstractSocket::SocketError socketError);
    void handleResponseTimeout();

private:
    void sendAttempt();
    void setState(State state);
    void finishWithError(Error error, const QString &message);
    void resetTransport();
    void clearOperationData();
    static QByteArray createRequestId();

    std::shared_ptr<Codec> m_codec;
    ClientConfig m_config;
    QUdpSocket m_socket;
    QTimer m_responseTimer;
    State m_state = State::Idle;
    Request m_request;
    QByteArray m_requestDatagram;
    int m_attempt = 0;
    bool m_active = false;
};

} // namespace spa

#endif // SPA_CLIENT_H
