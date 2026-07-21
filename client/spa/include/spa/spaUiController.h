#ifndef SPA_UI_CONTROLLER_H
#define SPA_UI_CONTROLLER_H

#include <QObject>

#include <memory>

#include "spa/spaClient.h"
#include "spa/spaConfig.h"

namespace spa {

class UiController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool succeeded READ succeeded NOTIFY succeededChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString statusType READ statusType NOTIFY statusTypeChanged)

public:
    explicit UiController(const QString &appVersion, QObject *parent = nullptr);

    bool busy() const;
    bool succeeded() const;
    QString statusMessage() const;
    QString statusType() const;
    LoginEndpoint loginEndpoint() const;

    Q_INVOKABLE void start(const QString &gatewayHost);
    Q_INVOKABLE void cancel();

signals:
    void busyChanged();
    void succeededChanged();
    void statusMessageChanged();
    void statusTypeChanged();
    void spaSucceeded();

private:
    static QString normalizedHost(const QString &input, QString *errorMessage);
    void setBusy(bool value);
    void setSucceeded(bool value);
    void setStatus(const QString &message, const QString &type);

    DefaultConfig m_defaultConfig;
    QString m_initializationError;
    std::shared_ptr<Codec> m_codec;
    std::unique_ptr<Client> m_client;
    LoginEndpoint m_loginEndpoint;
    bool m_busy = false;
    bool m_succeeded = false;
    QString m_statusMessage;
    QString m_statusType = QStringLiteral("info");
};

} // namespace spa

#endif // SPA_UI_CONTROLLER_H
