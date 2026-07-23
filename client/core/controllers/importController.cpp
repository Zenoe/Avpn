#include "importController.h"

#include <QDataStream>
#include <QIODevice>
#include <QJsonArray>
#include <QRegularExpression>
#include <QUrl>

#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/qrCodeUtils.h"
#include "core/utils/serverConfigUtils.h"

using namespace caelispect;

ImportController::ImportController(SecureServersRepository *serversRepository,
                                   SecureAppSettingsRepository *appSettingsRepository, QObject *parent)
    : QObject(parent), m_serversRepository(serversRepository), m_appSettingsRepository(appSettingsRepository)
{
}

ConfigTypes ImportController::checkConfigFormat(const QString &config)
{
    return config.contains(QStringLiteral("[Interface]")) && config.contains(QStringLiteral("[Peer]"))
            ? ConfigTypes::WireGuard : ConfigTypes::Invalid;
}

ImportController::ImportResult ImportController::extractConfigFromData(const QString &data, const QString &configFileName)
{
    ImportResult result;
    result.configFileName = configFileName;
    QString config = data;
    if (config.startsWith(QStringLiteral("vpn://"))) {
        result.errorCode = ErrorCode::ImportInvalidConfigError;
        return result;
    }

    if (checkConfigFormat(config) == ConfigTypes::Invalid) {
        QByteArray decoded = QByteArray::fromBase64(config.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
        const QByteArray uncompressed = qUncompress(decoded);
        if (!uncompressed.isEmpty()) decoded = uncompressed;
        config = QString::fromUtf8(decoded);
    }

    if (checkConfigFormat(config) != ConfigTypes::WireGuard) {
        result.errorCode = ErrorCode::ImportInvalidConfigError;
        result.configFileName.clear();
        return result;
    }

    result.config = extractWireGuardConfig(config);
    if (result.config.isEmpty()) {
        result.errorCode = ErrorCode::ImportInvalidConfigError;
        return result;
    }
    result.configType = ConfigTypes::WireGuard;
    result.isNativeWireGuardConfig = true;
    return result;
}

ImportController::ImportResult ImportController::extractConfigFromQr(const QByteArray &data)
{
    ImportResult result = extractConfigFromData(QString::fromUtf8(data));
    if (result.errorCode == ErrorCode::NoError) return result;

    QByteArray decoded = QByteArray::fromBase64(data, QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    const QByteArray uncompressed = qUncompress(decoded);
    if (!uncompressed.isEmpty()) decoded = uncompressed;
    return extractConfigFromData(QString::fromUtf8(decoded));
}

void ImportController::startDecodingQr()
{
    m_qrCodeChunks.clear();
    m_totalQrCodeChunksCount = 0;
    m_receivedQrCodeChunksCount = 0;
    m_isQrCodeProcessed = true;
}

ImportController::QrParseResult ImportController::parseQrCodeChunk(const QString &code)
{
    QrParseResult parsed { false, {}, m_receivedQrCodeChunksCount, m_totalQrCodeChunksCount };
    if (!m_isQrCodeProcessed) return parsed;

    QByteArray payload = QByteArray::fromBase64(code.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QDataStream stream(&payload, QIODevice::ReadOnly);
    qint16 magic = 0;
    stream >> magic;
    if (magic == qrCodeUtils::qrMagicCode) {
        quint8 count = 0;
        quint8 id = 0;
        stream >> count >> id >> m_qrCodeChunks[id];
        if (m_totalQrCodeChunksCount != count) m_qrCodeChunks.clear();
        m_totalQrCodeChunksCount = count;
        m_receivedQrCodeChunksCount = m_qrCodeChunks.size();
        parsed.chunksReceived = m_receivedQrCodeChunksCount;
        parsed.chunksTotal = m_totalQrCodeChunksCount;
        if (m_qrCodeChunks.size() != m_totalQrCodeChunksCount) return parsed;
        QByteArray all;
        for (int i = 0; i < m_totalQrCodeChunksCount; ++i) all.append(m_qrCodeChunks.value(i));
        parsed.importResult = extractConfigFromQr(all);
    } else {
        parsed.importResult = extractConfigFromQr(code.toUtf8());
        if (parsed.importResult.errorCode != ErrorCode::NoError) parsed.importResult = extractConfigFromQr(payload);
    }
    parsed.success = parsed.importResult.errorCode == ErrorCode::NoError;
    if (parsed.success) m_isQrCodeProcessed = false;
    return parsed;
}

bool ImportController::isQrDecodingActive() const { return m_isQrCodeProcessed; }
int ImportController::qrChunksReceived() const { return m_receivedQrCodeChunksCount; }
int ImportController::qrChunksTotal() const { return m_totalQrCodeChunksCount; }

void ImportController::importConfig(const QJsonObject &config)
{
    if (serverConfigUtils::configTypeFromJson(config) != serverConfigUtils::ConfigType::WireGuardProfile) {
        emit importErrorOccurred(ErrorCode::ImportInvalidConfigError, false);
        return;
    }
    m_serversRepository->addServer({}, config, serverConfigUtils::ConfigType::WireGuardProfile);
    emit importFinished();
}

QJsonObject ImportController::extractWireGuardConfig(const QString &data) const
{
    QMap<QString, QString> interfaceValues;
    QMap<QString, QString> peerValues;
    QMap<QString, QString> *values = nullptr;
    for (const QString &sourceLine : data.split('\n')) {
        const QString line = sourceLine.trimmed();
        if (line == QStringLiteral("[Interface]")) { values = &interfaceValues; continue; }
        if (line == QStringLiteral("[Peer]")) { values = &peerValues; continue; }
        if (!values || line.isEmpty() || line.startsWith('#') || line.startsWith(';')) continue;
        const int equals = line.indexOf('=');
        if (equals > 0) (*values)[line.left(equals).trimmed()] = line.mid(equals + 1).trimmed();
    }

    const QStringList awgFields { "Jc", "Jmin", "Jmax", "S1", "S2", "H1", "H2", "H3", "H4" };
    for (const QString &field : awgFields) {
        if (interfaceValues.contains(field) || peerValues.contains(field)) return {};
    }

    const QString endpoint = peerValues.value(protocols::wireguard::Endpoint);
    const QUrl endpointUrl = QUrl::fromUserInput(QStringLiteral("udp://") + endpoint);
    if (endpointUrl.host().isEmpty() || endpointUrl.port() < 1
            || interfaceValues.value(protocols::wireguard::PrivateKey).isEmpty()
            || interfaceValues.value(protocols::wireguard::Address).isEmpty()
            || peerValues.value(protocols::wireguard::PublicKey).isEmpty()) return {};

    QJsonObject wireguard;
    wireguard[configKey::config] = data;
    wireguard[configKey::hostName] = endpointUrl.host();
    wireguard[configKey::port] = endpointUrl.port();
    wireguard[configKey::clientPrivKey] = interfaceValues.value(protocols::wireguard::PrivateKey);
    wireguard[configKey::clientIp] = interfaceValues.value(protocols::wireguard::Address);
    wireguard[configKey::serverPubKey] = peerValues.value(protocols::wireguard::PublicKey);
    wireguard[configKey::mtu] = interfaceValues.value(protocols::wireguard::MTU, protocols::wireguard::defaultMtu);
    wireguard[configKey::persistentKeepAlive] = peerValues.value(protocols::wireguard::PersistentKeepalive);
    const QString psk = peerValues.value(protocols::wireguard::PresharedKey, peerValues.value(protocols::wireguard::PreSharedKey));
    if (!psk.isEmpty()) wireguard[configKey::pskKey] = psk;
    QJsonArray allowedIps;
    for (const QString &ip : peerValues.value(protocols::wireguard::AllowedIPs).split(',', Qt::SkipEmptyParts)) allowedIps.append(ip.trimmed());
    if (allowedIps.isEmpty()) return {};
    wireguard[configKey::allowedIps] = allowedIps;

    QJsonObject profile;
    profile[QStringLiteral("profileType")] = QStringLiteral("wireguard");
    profile[configKey::description] = m_serversRepository->nextAvailableServerName();
    profile[configKey::hostName] = endpointUrl.host();
    profile[configKey::wireguard] = wireguard;
    const QStringList dns = interfaceValues.value(QStringLiteral("DNS")).split(',', Qt::SkipEmptyParts);
    if (!dns.isEmpty()) profile[configKey::dns1] = dns.first().trimmed();
    if (dns.size() > 1) profile[configKey::dns2] = dns.at(1).trimmed();
    return profile;
}
