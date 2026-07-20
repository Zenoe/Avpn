#include "importController.h"

#include <QDataStream>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QIODevice>
#include <QMap>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <QUrl>
#include <algorithm>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/serverConfigUtils.h"
#include "core/utils/utilities.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/qrCodeUtils.h"

using namespace amnezia;
using namespace ProtocolUtils;

namespace
{
    ConfigTypes checkConfigFormat(const QString &config)
    {
        const QString wireguardConfigPatternSectionInterface = "[Interface]";
        const QString wireguardConfigPatternSectionPeer = "[Peer]";

        const QString amneziaConfigPattern = "containers";
        const QString amneziaConfigPatternHostName = "hostName";
        const QString amneziaConfigPatternUserName = "userName";
        const QString amneziaConfigPatternPassword = "password";
        const QString backupPattern = "Servers/serversList";

        if (config.contains(backupPattern)) {
            return ConfigTypes::Backup;
        } else if (config.contains(amneziaConfigPattern)
                   || (config.contains(amneziaConfigPatternHostName) && config.contains(amneziaConfigPatternUserName)
                       && config.contains(amneziaConfigPatternPassword))) {
            return ConfigTypes::Amnezia;
        } else if (config.contains(wireguardConfigPatternSectionInterface) && config.contains(wireguardConfigPatternSectionPeer)) {
            return ConfigTypes::WireGuard;
        }
        return ConfigTypes::Invalid;
    }
} // namespace

ImportController::ImportController(SecureServersRepository* serversRepository,
                                   SecureAppSettingsRepository* appSettingsRepository,
                                   QObject *parent)
    : QObject(parent),
      m_serversRepository(serversRepository),
      m_appSettingsRepository(appSettingsRepository)
{
}

ImportController::ImportResult ImportController::extractConfigFromData(const QString &data, const QString &configFileName)
{
    ImportResult result;
    result.configFileName = configFileName;
    result.maliciousWarningText.clear();

    QString config = data;
    if (config.startsWith("vpn://")) {
        result.errorCode = ErrorCode::ImportInvalidConfigError;
        return result;
    }
    ConfigTypes configType = ConfigTypes::Invalid;

    configType = checkConfigFormat(config);
    if (configType == ConfigTypes::Invalid) {
        QByteArray ba = QByteArray::fromBase64(config.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
        QByteArray baUncompressed = qUncompress(ba);
        if (!baUncompressed.isEmpty()) {
            ba = baUncompressed;
        }

        config = ba;
        configType = checkConfigFormat(config);
    }

    result.configType = configType;

    switch (configType) {
    case ConfigTypes::WireGuard: {
        result.config = extractWireGuardConfig(config, result.configType);
        result.isNativeWireGuardConfig = (result.configType == ConfigTypes::WireGuard);
        if (!result.config.empty()) {
            return result;
        }
        result.errorCode = ErrorCode::ImportInvalidConfigError;
        return result;
    }
    case ConfigTypes::Amnezia: {
        result.config = QJsonDocument::fromJson(config.toUtf8()).object();

        if (result.config.contains(QStringLiteral("api_key"))
            || result.config.contains(QStringLiteral("auth_data"))
            || serverConfigUtils::configTypeFromJson(result.config) == serverConfigUtils::ConfigType::Invalid) {
            result.errorCode = ErrorCode::ImportInvalidConfigError;
            result.config = {};
            return result;
        }

        processAmneziaConfig(result.config);
        if (!result.config.empty()) {
            return result;
        }
        result.errorCode = ErrorCode::ImportInvalidConfigError;
        return result;
    }
    case ConfigTypes::Backup: {
        result.errorCode = ErrorCode::ImportBackupFileUseRestoreInstead;
        return result;
    }
    case ConfigTypes::Invalid: {
        result.errorCode = ErrorCode::ImportInvalidConfigError;
        result.configFileName.clear();
        return result;
    }
    }
    
    result.errorCode = ErrorCode::ImportInvalidConfigError;
    return result;
}

ImportController::ImportResult ImportController::extractConfigFromQr(const QByteArray &data)
{
    ImportResult result;

    QString dataStr = QString::fromUtf8(data);
    ConfigTypes configType = checkConfigFormat(dataStr);
    if (configType != ConfigTypes::Invalid) {
        return extractConfigFromData(dataStr, "");
    }

    QJsonObject dataObj = QJsonDocument::fromJson(data).object();
    if (!dataObj.isEmpty()) {
        result.config = dataObj;
        result.configType = ConfigTypes::Amnezia;
        return result;
    }

    QByteArray ba_uncompressed = qUncompress(data);
    if (!ba_uncompressed.isEmpty()) {
        result.config = QJsonDocument::fromJson(ba_uncompressed).object();
        if (result.config.isEmpty()) {
            result.errorCode = ErrorCode::ImportInvalidConfigError;
            return result;
        }
        result.configType = ConfigTypes::Amnezia;
        return result;
    }

    QByteArray ba = QByteArray::fromBase64(data, QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QByteArray baUncompressed = qUncompress(ba);

    if (!baUncompressed.isEmpty()) {
        ba = baUncompressed;
    }

    if (!ba.isEmpty()) {
        result.config = QJsonDocument::fromJson(ba).object();
        if (result.config.isEmpty()) {
            result.errorCode = ErrorCode::ImportInvalidConfigError;
            return result;
        }
        result.configType = ConfigTypes::Amnezia;
        return result;
    }

    result.errorCode = ErrorCode::ImportInvalidConfigError;
    return result;
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
    QrParseResult parseResult;
    parseResult.chunksReceived = m_receivedQrCodeChunksCount;
    parseResult.chunksTotal = m_totalQrCodeChunksCount;

    if (!m_isQrCodeProcessed) {
        return parseResult;
    }

    QByteArray ba = QByteArray::fromBase64(code.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QDataStream s(&ba, QIODevice::ReadOnly);
    qint16 magic;
    s >> magic;

    if (magic == qrCodeUtils::qrMagicCode) {
        quint8 chunksCount;
        s >> chunksCount;
        if (m_totalQrCodeChunksCount != chunksCount) {
            m_qrCodeChunks.clear();
        }

        m_totalQrCodeChunksCount = chunksCount;

        quint8 chunkId;
        s >> chunkId;
        s >> m_qrCodeChunks[chunkId];
        m_receivedQrCodeChunksCount = m_qrCodeChunks.size();
        parseResult.chunksReceived = m_receivedQrCodeChunksCount;
        parseResult.chunksTotal = m_totalQrCodeChunksCount;

        if (m_qrCodeChunks.size() == m_totalQrCodeChunksCount) {
            QByteArray data;
            for (int i = 0; i < m_totalQrCodeChunksCount; ++i) {
                data.append(m_qrCodeChunks.value(i));
            }

            ImportResult result = extractConfigFromQr(data);
            if (result.errorCode == ErrorCode::NoError) {
                parseResult.success = true;
                parseResult.importResult = result;
                m_isQrCodeProcessed = false;
            } else {
                m_qrCodeChunks.clear();
                m_totalQrCodeChunksCount = 0;
                m_receivedQrCodeChunksCount = 0;
            }
        }
    } else {
        ImportResult result = extractConfigFromQr(code.toUtf8());
        if (result.errorCode != ErrorCode::NoError) {
            result = extractConfigFromQr(ba);
        }
        if (result.errorCode == ErrorCode::NoError) {
            parseResult.success = true;
            parseResult.importResult = result;
            m_isQrCodeProcessed = false;
        }
    }

    return parseResult;
}

bool ImportController::isQrDecodingActive() const
{
    return m_isQrCodeProcessed;
}

int ImportController::qrChunksReceived() const
{
    return m_receivedQrCodeChunksCount;
}

int ImportController::qrChunksTotal() const
{
    return m_totalQrCodeChunksCount;
}

void ImportController::importConfig(const QJsonObject &config)
{
    ServerCredentials credentials;
    credentials.hostName = config.value(configKey::hostName).toString();
    credentials.port = config.value(configKey::port).toInt();
    credentials.userName = config.value(configKey::userName).toString();
    credentials.secretData = config.value(configKey::password).toString();

    if (credentials.isValid() || config.contains(configKey::containers)) {
        m_serversRepository->addServer(QString(), config, serverConfigUtils::configTypeFromJson(config));
        emit importFinished();
    } else {
        qDebug() << "Failed to import profile";
        qDebug().noquote() << QJsonDocument(config).toJson();
        emit importErrorOccurred(ErrorCode::ImportInvalidConfigError, false);
    }
}

ConfigTypes ImportController::checkConfigFormat(const QString &config) const
{
    return ::checkConfigFormat(config);
}

QJsonObject ImportController::extractWireGuardConfig(const QString &data, ConfigTypes &configType) const
{
    QMap<QString, QString> configMap;
    auto configByLines = data.split("\n");
    for (const QString &line : configByLines) {
        QString trimmedLine = line.trimmed();
        if (trimmedLine.startsWith("[") && trimmedLine.endsWith("]")) {
            continue;
        } else {
            QStringList parts = trimmedLine.split(" = ");
            if (parts.count() == 2) {
                configMap[parts.at(0).trimmed()] = parts.at(1).trimmed();
            }
        }
    }

    QJsonObject lastConfig;
    lastConfig[configKey::config] = data;

    auto url { QUrl::fromUserInput(configMap.value(protocols::wireguard::Endpoint)) };
    QString hostName;
    QString port;
    if (!url.host().isEmpty()) {
        hostName = url.host();
    } else {
        qDebug() << "Key parameter" << protocols::wireguard::Endpoint << "is missing or has an invalid format";
        return QJsonObject();
    }

    if (url.port() != -1) {
        port = QString::number(url.port());
    } else {
        port = protocols::wireguard::defaultPort;
    }

    lastConfig[configKey::hostName] = hostName;
    lastConfig[configKey::port] = port.toInt();

    if (!configMap.value(protocols::wireguard::PrivateKey).isEmpty()
            && !configMap.value(protocols::wireguard::Address).isEmpty()
            && !configMap.value(protocols::wireguard::PublicKey).isEmpty()) {
        lastConfig[configKey::clientPrivKey] = configMap.value(protocols::wireguard::PrivateKey);
        lastConfig[configKey::clientIp] = configMap.value(protocols::wireguard::Address);

        if (!configMap.value(protocols::wireguard::PresharedKey).isEmpty()) {
            lastConfig[configKey::pskKey] = configMap.value(protocols::wireguard::PresharedKey);
        } else if (!configMap.value(protocols::wireguard::PreSharedKey).isEmpty()) {
            lastConfig[configKey::pskKey] = configMap.value(protocols::wireguard::PreSharedKey);
        }

        lastConfig[configKey::serverPubKey] = configMap.value(protocols::wireguard::PublicKey);
    } else {
        qDebug() << "One of the key parameters is missing (PrivateKey, Address, PublicKey)";
        return QJsonObject();
    }

    if (!configMap.value(protocols::wireguard::MTU).isEmpty()) {
        lastConfig[configKey::mtu] = configMap.value(protocols::wireguard::MTU);
    }

    if (!configMap.value(protocols::wireguard::PersistentKeepalive).isEmpty()) {
        lastConfig[configKey::persistentKeepAlive] = configMap.value(protocols::wireguard::PersistentKeepalive);
    }

    QJsonArray allowedIpsJsonArray = QJsonArray::fromStringList(
                configMap.value(protocols::wireguard::AllowedIPs).split(", "));

    lastConfig[configKey::allowedIps] = allowedIpsJsonArray;

    QString protocolName = configKey::wireguard;
    ConfigTypes detectedType = ConfigTypes::WireGuard;

    const QStringList requiredJunkFields = { QStringLiteral("Jc"), QStringLiteral("Jmin"), QStringLiteral("Jmax"),
                                             QStringLiteral("S1"), QStringLiteral("S2"), QStringLiteral("H1"),
                                             QStringLiteral("H2"), QStringLiteral("H3"), QStringLiteral("H4") };

    bool hasAllRequiredFields = std::all_of(requiredJunkFields.begin(), requiredJunkFields.end(),
                                            [&configMap](const QString &field) { return !configMap.value(field).isEmpty(); });
    if (hasAllRequiredFields) {
        return QJsonObject();
    }

    if (!configMap.value(protocols::wireguard::MTU).isEmpty()) {
        lastConfig[configKey::mtu] = configMap.value(protocols::wireguard::MTU);
    } else {
        lastConfig[configKey::mtu] = protocols::wireguard::defaultMtu;
    }

    QJsonObject wireguardConfig;
    wireguardConfig[configKey::lastConfig] = QString(QJsonDocument(lastConfig).toJson());
    wireguardConfig[configKey::isThirdPartyConfig] = true;
    wireguardConfig[configKey::port] = port;
    wireguardConfig[configKey::transportProto] = QStringLiteral("udp");

    QJsonObject containers;
    QString containerName = configKey::amneziaWireguard;
    containers.insert(configKey::container, QJsonValue(containerName));
    containers.insert(protocolName, QJsonValue(wireguardConfig));

    QJsonArray arr;
    arr.push_back(containers);

    QJsonObject config;
    config[configKey::containers] = arr;
    config[configKey::defaultContainer] = containerName;
    config[configKey::description] = m_serversRepository->nextAvailableServerName();

    const static QRegularExpression dnsRegExp(
            "DNS = "
            "(\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b).*(\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b)");
    QRegularExpressionMatch dnsMatch = dnsRegExp.match(data);
    if (dnsMatch.hasMatch()) {
        config[configKey::dns1] = dnsMatch.captured(1);
        config[configKey::dns2] = dnsMatch.captured(2);
    }

    config[configKey::hostName] = hostName;

    configType = detectedType;
    return config;
}

void ImportController::processAmneziaConfig(QJsonObject &config) const
{
    auto containers = config.value(configKey::containers).toArray();
    QJsonArray supportedContainers;
    for (const auto &value : std::as_const(containers)) {
        const auto object = value.toObject();
        const auto container = ContainerUtils::containerFromString(object.value(configKey::container).toString());
        if (container == DockerContainer::WireGuard) {
            supportedContainers.append(object);
        }
    }
    containers = supportedContainers;
    config[configKey::containers] = containers;
    if (!containers.isEmpty()) {
        config[configKey::defaultContainer] = containers.first().toObject().value(configKey::container);
    }
    for (auto i = 0; i < containers.size(); i++) {
        auto container = containers.at(i).toObject();
        auto dockerContainer = ContainerUtils::containerFromString(container.value(configKey::container).toString());
        if (dockerContainer == DockerContainer::WireGuard) {
            auto containerConfig = container.value(ContainerUtils::containerTypeToProtocolString(dockerContainer)).toObject();
            auto protocolConfig = containerConfig.value(configKey::lastConfig).toString();
            if (protocolConfig.isEmpty()) {
                return;
            }

            QJsonObject jsonConfig = QJsonDocument::fromJson(protocolConfig.toUtf8()).object();
            jsonConfig[configKey::mtu] = protocols::wireguard::defaultMtu;

            containerConfig[configKey::lastConfig] = QString(QJsonDocument(jsonConfig).toJson());

            container[ContainerUtils::containerTypeToProtocolString(dockerContainer)] = containerConfig;
            containers.replace(i, container);
            config.insert(configKey::containers, containers);
        }
    }
}

