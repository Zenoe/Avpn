#ifndef IMPORTCONTROLLER_H
#define IMPORTCONTROLLER_H

#include <QByteArray>
#include <QJsonObject>
#include <QMap>
#include <QObject>

#include "core/repositories/secureAppSettingsRepository.h"
#include "core/repositories/secureServersRepository.h"
#include "core/utils/errorCodes.h"

namespace
{
enum class ConfigTypes { WireGuard, Invalid };
}

class ImportController : public QObject
{
    Q_OBJECT
public:
    struct ImportResult {
        caelispect::ErrorCode errorCode = caelispect::ErrorCode::NoError;
        QJsonObject config;
        QString configFileName;
        QString maliciousWarningText;
        ConfigTypes configType = ConfigTypes::Invalid;
        bool isNativeWireGuardConfig = false;
    };

    explicit ImportController(SecureServersRepository *serversRepository,
                              SecureAppSettingsRepository *appSettingsRepository, QObject *parent = nullptr);

    struct QrParseResult {
        bool success = false;
        ImportResult importResult;
        int chunksReceived = 0;
        int chunksTotal = 0;
    };

    ImportResult extractConfigFromData(const QString &data, const QString &configFileName = "");
    ImportResult extractConfigFromQr(const QByteArray &data);
    void startDecodingQr();
    QrParseResult parseQrCodeChunk(const QString &code);
    bool isQrDecodingActive() const;
    int qrChunksReceived() const;
    int qrChunksTotal() const;
    void importConfig(const QJsonObject &config);

signals:
    void importFinished();
    void importErrorOccurred(caelispect::ErrorCode errorCode, bool goToPageHome);

private:
    static ConfigTypes checkConfigFormat(const QString &config);
    QJsonObject extractWireGuardConfig(const QString &data) const;

    SecureServersRepository *m_serversRepository;
    SecureAppSettingsRepository *m_appSettingsRepository;
    QMap<int, QByteArray> m_qrCodeChunks;
    bool m_isQrCodeProcessed = false;
    int m_totalQrCodeChunksCount = 0;
    int m_receivedQrCodeChunksCount = 0;
};

#endif // IMPORTCONTROLLER_H
