#include "spa/spaConfig.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

void initializeSpaResources()
{
    Q_INIT_RESOURCE(spa_config);
}

namespace {

void setError(QString *errorMessage, const QString &message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
}

spa::KeyConfig readKey(const QJsonObject &object, QString *errorMessage)
{
    spa::KeyConfig key;
    const int keyId = object.value(QLatin1String("keyId")).toInt();
    if (keyId <= 0 || keyId > 65535) {
        setError(errorMessage, QStringLiteral("SPA keyId must be between 1 and 65535"));
        return {};
    }

    key.keyId = static_cast<quint16>(keyId);
    key.name = object.value(QLatin1String("name")).toString();
    key.publicKey = QByteArray::fromHex(object.value(QLatin1String("publicKey")).toString().toLatin1());
    if (key.publicKey.size() != 65 || static_cast<quint8>(key.publicKey.front()) != 0x04) {
        setError(errorMessage, QStringLiteral("SPA SM2 public key must be a 65-byte uncompressed point"));
        return {};
    }
    return key;
}

} // namespace

namespace spa {

DefaultConfig ConfigLoader::loadDefault(QString *errorMessage)
{
    initializeSpaResources();
    QFile file(QStringLiteral(":/spa/resources/spa-config.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        setError(errorMessage, QStringLiteral("Unable to open the embedded SPA configuration"));
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setError(errorMessage, QStringLiteral("Invalid embedded SPA configuration: %1").arg(parseError.errorString()));
        return {};
    }

    DefaultConfig result;
    const QJsonObject root = document.object();
    const QJsonObject gateway = root.value(QLatin1String("gateway")).toObject();
    const int udpPort = gateway.value(QLatin1String("udpPort")).toInt();
    if (udpPort <= 0 || udpPort > 65535) {
        setError(errorMessage, QStringLiteral("Invalid SPA gateway UDP port"));
        return {};
    }
    result.gatewayPort = static_cast<quint16>(udpPort);

    const QJsonObject protocol = root.value(QLatin1String("protocol")).toObject();
    const int version = protocol.value(QLatin1String("version")).toInt();
    if (version <= 0 || version > 255) {
        setError(errorMessage, QStringLiteral("Invalid SPA protocol version"));
        return {};
    }
    result.protocol.version = static_cast<quint8>(version);
    result.protocol.cipherSuite = protocol.value(QLatin1String("cipherSuite")).toString();
    result.protocol.sm2Id = protocol.value(QLatin1String("sm2Id")).toString().toUtf8();
    result.protocol.maximumClockSkewMsecs = protocol.value(QLatin1String("maximumClockSkewMsecs")).toInt(60000);

    const QJsonObject keys = root.value(QLatin1String("keys")).toObject();
    result.protocol.encryptionKey = readKey(keys.value(QLatin1String("encryption")).toObject(), errorMessage);
    if (result.protocol.encryptionKey.publicKey.isEmpty()) {
        return {};
    }
    result.protocol.signingKey = readKey(keys.value(QLatin1String("signing")).toObject(), errorMessage);
    if (result.protocol.signingKey.publicKey.isEmpty()) {
        return {};
    }

    const QJsonObject policy = root.value(QLatin1String("endpointPolicy")).toObject();
    for (const QJsonValue &scheme : policy.value(QLatin1String("allowedSchemes")).toArray()) {
        result.client.endpointPolicy.allowedSchemes.append(scheme.toString());
    }
    result.client.endpointPolicy.minimumPort = static_cast<quint16>(policy.value(QLatin1String("minimumPort")).toInt(1));
    result.client.endpointPolicy.maximumPort = static_cast<quint16>(policy.value(QLatin1String("maximumPort")).toInt(65535));
    result.client.endpointPolicy.minimumRemainingLifetimeMsecs =
            policy.value(QLatin1String("minimumRemainingLifetimeMsecs")).toInt(1000);

    const QJsonObject network = root.value(QLatin1String("network")).toObject();
    result.client.gatewayPort = result.gatewayPort;
    result.client.responseTimeoutMsecs = network.value(QLatin1String("responseTimeoutMsecs")).toInt(3000);
    result.client.maximumAttempts = network.value(QLatin1String("maximumAttempts")).toInt(3);
    result.client.maximumResponseBytes = network.value(QLatin1String("maximumResponseBytes")).toInt(16 * 1024);

    QString clientError;
    result.client.gatewayHost = QStringLiteral("configuration-validation-placeholder");
    if (result.protocol.cipherSuite != QLatin1String("GM-SPA-SM2-ECDH-SM4GCM-SM3-V1")
        || result.protocol.sm2Id.isEmpty() || result.protocol.maximumClockSkewMsecs <= 0
        || !result.client.isValid(&clientError)) {
        setError(errorMessage, clientError.isEmpty() ? QStringLiteral("Unsupported SPA configuration") : clientError);
        return {};
    }
    result.client.gatewayHost.clear();
    return result;
}

} // namespace spa
