#include "usersController.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/selfhosted/scriptsRegistry.h"
#include "logger.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/models/containerConfig.h"

using namespace amnezia;

namespace
{
    Logger logger("UsersController");
}

UsersController::UsersController(SecureServersRepository* serversRepository, QObject *parent)
    : QObject(parent),
      m_serversRepository(serversRepository)
{
}

bool UsersController::isClientExists(const QString &clientId, const QJsonArray &clientsTable)
{
    for (const QJsonValue &value : std::as_const(clientsTable)) {
        if (value.isObject()) {
            QJsonObject obj = value.toObject();
            if (obj.contains(configKey::clientId) && obj[configKey::clientId].toString() == clientId) {
                return true;
            }
        }
    }
    return false;
}

int UsersController::clientIndexById(const QString &clientId, const QJsonArray &clientsTable)
{
    for (int i = 0; i < clientsTable.size(); ++i) {
        if (clientsTable.at(i).isObject()) {
            QJsonObject obj = clientsTable.at(i).toObject();
            if (obj.contains(configKey::clientId) && obj[configKey::clientId].toString() == clientId) {
                return i;
            }
        }
    }
    return -1;
}

void UsersController::migration(const QByteArray &clientsTableString, QJsonArray &clientsTable)
{
    QJsonObject clientsTableObj = QJsonDocument::fromJson(clientsTableString).object();

    for (auto &clientId : clientsTableObj.keys()) {
        QJsonObject client;
        client[configKey::clientId] = clientId;

        QJsonObject userData;
        userData[configKey::clientName] = clientsTableObj.value(clientId).toObject().value(configKey::clientName);
        client[configKey::userData] = userData;

        clientsTable.push_back(client);
    }
}

ErrorCode UsersController::wgShow(const DockerContainer container, const ServerCredentials &credentials,
                                             SshSession* sshSession, std::vector<WgShowData> &data)
{
    if (container != DockerContainer::WireGuard && !ContainerUtils::isAwgContainer(container)) {
        return ErrorCode::NoError;
    }

    ErrorCode error = ErrorCode::NoError;
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    QString showBin = (container == DockerContainer::Awg2)
                       ? QStringLiteral("awg")
                       : QStringLiteral("wg");
    const QString command = QString("sudo docker exec -i $CONTAINER_NAME bash -c '%1 show all'").arg(showBin);

    QString script = sshSession->replaceVars(command, amnezia::genBaseVars(credentials, container, QString(), QString()));
    error = sshSession->runScript(credentials, script, cbReadStdOut);
    if (error != ErrorCode::NoError) {
        logger.error() << QString("Failed to execute %1 show command").arg(showBin);
        return error;
    }

    if (stdOut.isEmpty()) {
        return error;
    }

    const auto getStrValue = [](const auto str) { return str.mid(str.indexOf(":") + 1).trimmed(); };

    const auto parts = stdOut.split('\n');
    const auto peerList = parts.filter("peer:");
    const auto latestHandshakeList = parts.filter("latest handshake:");
    const auto transferredDataList = parts.filter("transfer:");
    const auto allowedIpsList = parts.filter("allowed ips:");

    if (allowedIpsList.isEmpty() || latestHandshakeList.isEmpty() || transferredDataList.isEmpty() || peerList.isEmpty()) {
        return error;
    }

    const auto changeHandshakeFormat = [](QString &latestHandshake) {
        const std::vector<std::pair<QString, QString>> replaceMap = { { " days", "d" },    { " hours", "h" }, { " minutes", "m" },
                                                                      { " seconds", "s" }, { " day", "d" },   { " hour", "h" },
                                                                      { " minute", "m" },  { " second", "s" } };

        for (const auto &item : replaceMap) {
            latestHandshake.replace(item.first, item.second);
        }
    };

    for (int i = 0; i < peerList.size() && i < transferredDataList.size() && i < latestHandshakeList.size() && i < allowedIpsList.size(); ++i) {

        const auto transferredData = getStrValue(transferredDataList[i]).split(",");
        auto latestHandshake = getStrValue(latestHandshakeList[i]);
        auto serverBytesReceived = transferredData.front().trimmed();
        auto serverBytesSent = transferredData.back().trimmed();
        auto allowedIps = getStrValue(allowedIpsList[i]);

        changeHandshakeFormat(latestHandshake);

        serverBytesReceived.chop(QStringLiteral(" received").length());
        serverBytesSent.chop(QStringLiteral(" sent").length());

        data.push_back({ getStrValue(peerList[i]), latestHandshake, serverBytesSent, serverBytesReceived, allowedIps });
    }

    return error;
}

ErrorCode UsersController::getWireGuardClients(const DockerContainer container, const ServerCredentials &credentials,
                                                          SshSession* sshSession, int &count, QJsonArray &clientsTable)
{
    ErrorCode error = ErrorCode::NoError;

    QString configPath;
    if (container == DockerContainer::Awg) {
        configPath = QString::fromLatin1(protocols::awg::serverLegacyConfigPath);
    } else if (container == DockerContainer::Awg2) {
        configPath = QString::fromLatin1(protocols::awg::serverConfigPath);
    } else {
        configPath = QString::fromLatin1(protocols::wireguard::serverConfigPath);
    }
    const QString wireguardConfigString = sshSession->getTextFileFromContainer(container, credentials, configPath, error);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to get the wg conf file from the server";
        return error;
    }

    auto configLines = wireguardConfigString.split("\n", Qt::SkipEmptyParts);
    QStringList wireguardKeys;
    for (const auto &line : configLines) {
        auto configPair = line.split(" = ", Qt::SkipEmptyParts);
        if (configPair.front() == "PublicKey") {
            wireguardKeys.push_back(configPair.back());
        }
    }

    for (auto &wireguardKey : wireguardKeys) {
        if (!isClientExists(wireguardKey, clientsTable)) {
            QJsonObject client;
            client[configKey::clientId] = wireguardKey;

            QJsonObject userData;
            userData[configKey::clientName] = QString("Client %1").arg(count);
            client[configKey::userData] = userData;

            clientsTable.push_back(client);

            count++;
        }
    }
    return error;
}

ErrorCode UsersController::updateClients(const QString &serverId, const DockerContainer container)
{
    ErrorCode error = ErrorCode::NoError;
    SshSession sshSession;
    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        return ErrorCode::InternalError;
    }
    ServerCredentials credentials = adminConfig->credentials();
    if (!credentials.isValid()) {
        return ErrorCode::InternalError;
    }

    const QString clientsTableFile = QString("/opt/amnezia/%1/clientsTable")
                                         .arg(ContainerUtils::containerTypeToString(container));

    const QByteArray clientsTableString = sshSession.getTextFileFromContainer(container, credentials, clientsTableFile, error);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to get the clientsTable file from the server";
        emit clientsUpdated(QJsonArray());
        return error;
    }

    m_clientsTable = QJsonDocument::fromJson(clientsTableString).array();

    if (m_clientsTable.isEmpty()) {
        migration(clientsTableString, m_clientsTable);

        int count = 0;

        if (container == DockerContainer::WireGuard || ContainerUtils::isAwgContainer(container)) {
            error = getWireGuardClients(container, credentials, &sshSession, count, m_clientsTable);
        } else {
            return ErrorCode::InternalError;
        }
        if (error != ErrorCode::NoError) {
            emit clientsUpdated(QJsonArray());
            return error;
        }

        const QByteArray newClientsTableString = QJsonDocument(m_clientsTable).toJson();
        if (clientsTableString != newClientsTableString) {
            error = sshSession.uploadTextFileToContainer(container, credentials, newClientsTableString, clientsTableFile);
            if (error != ErrorCode::NoError) {
                logger.error() << "Failed to upload the clientsTable file to the server";
            }
        }
    }

    std::vector<WgShowData> data;
    wgShow(container, credentials, &sshSession, data);

    for (const auto &client : data) {
        int i = 0;
        for (const auto &it : std::as_const(m_clientsTable)) {
            if (it.isObject()) {
                QJsonObject obj = it.toObject();
                if (obj.contains(configKey::clientId) && obj[configKey::clientId].toString() == client.clientId) {
                    QJsonObject userData = obj[configKey::userData].toObject();

                    if (!client.latestHandshake.isEmpty()) {
                        userData[configKey::latestHandshake] = client.latestHandshake;
                    }

                    if (!client.dataReceived.isEmpty()) {
                        userData[configKey::dataReceived] = client.dataReceived;
                    }

                    if (!client.dataSent.isEmpty()) {
                        userData[configKey::dataSent] = client.dataSent;
                    }

                    if (!client.allowedIps.isEmpty()) {
                        userData[configKey::allowedIps] = client.allowedIps;
                    }

                    obj[configKey::userData] = userData;
                    m_clientsTable.replace(i, obj);
                    break;
                }
            }
            ++i;
        }
    }

    emit clientsUpdated(m_clientsTable);
    return error;
}


ErrorCode UsersController::appendClient(const QString &serverId, const QString &clientId, const QString &clientName, const DockerContainer container)
{
    ErrorCode error = ErrorCode::NoError;
    SshSession sshSession;
    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        return ErrorCode::InternalError;
    }
    ServerCredentials credentials = adminConfig->credentials();
    if (!credentials.isValid()) {
        return ErrorCode::InternalError;
    }

    error = updateClients(serverId, container);
    if (error != ErrorCode::NoError) {
        return error;
    }

    int existingIndex = clientIndexById(clientId, m_clientsTable);
    if (existingIndex >= 0) {
        return renameClient(serverId, existingIndex, clientName, container, true);
    }

    QJsonObject client;
    client[configKey::clientId] = clientId;

    QJsonObject userData;
    userData[configKey::clientName] = clientName;
    userData[configKey::creationDate] = QDateTime::currentDateTime().toString();
    client[configKey::userData] = userData;
    m_clientsTable.push_back(client);

    const QByteArray clientsTableString = QJsonDocument(m_clientsTable).toJson();

    const QString clientsTableFile = QString("/opt/amnezia/%1/clientsTable")
                                         .arg(ContainerUtils::containerTypeToString(container));

    error = sshSession.uploadTextFileToContainer(container, credentials, clientsTableString, clientsTableFile);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to upload the clientsTable file to the server";
        return error;
    }

    emit clientAdded(client);
    emit clientsUpdated(m_clientsTable);
    return error;
}

ErrorCode UsersController::renameClient(const QString &serverId, const int row, const QString &clientName,
                                                   const DockerContainer container, bool addTimeStamp)
{
    if (row < 0 || row >= m_clientsTable.size()) {
        return ErrorCode::InternalError;
    }

    SshSession sshSession;
    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        return ErrorCode::InternalError;
    }
    ServerCredentials credentials = adminConfig->credentials();
    if (!credentials.isValid()) {
        return ErrorCode::InternalError;
    }

    auto client = m_clientsTable.at(row).toObject();
    auto userData = client[configKey::userData].toObject();
    userData[configKey::clientName] = clientName;
    if (addTimeStamp) {
        userData[configKey::creationDate] = QDateTime::currentDateTime().toString();
    }
    client[configKey::userData] = userData;

    m_clientsTable.replace(row, client);

    const QByteArray clientsTableString = QJsonDocument(m_clientsTable).toJson();

    const QString clientsTableFile = QString("/opt/amnezia/%1/clientsTable")
                                         .arg(ContainerUtils::containerTypeToString(container));

    ErrorCode error = sshSession.uploadTextFileToContainer(container, credentials, clientsTableString, clientsTableFile);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to upload the clientsTable file to the server";
        return error;
    }

    if (addTimeStamp) {
        emit clientsUpdated(m_clientsTable);
    } else {
        emit clientRenamed(row, clientName);
    }
    return error;
}

ErrorCode UsersController::revokeWireGuard(const int row, const DockerContainer container, const ServerCredentials &credentials,
                                                      SshSession* sshSession, QJsonArray &clientsTable)
{
    if (row < 0 || row >= clientsTable.size()) {
        return ErrorCode::InternalError;
    }

    ErrorCode error = ErrorCode::NoError;

    QString configPath;
    if (container == DockerContainer::Awg) {
        configPath = QString::fromLatin1(protocols::awg::serverLegacyConfigPath);
    } else if (container == DockerContainer::Awg2) {
        configPath = QString::fromLatin1(protocols::awg::serverConfigPath);
    } else {
        configPath = QString::fromLatin1(protocols::wireguard::serverConfigPath);
    }
    const QString wireguardConfigString = sshSession->getTextFileFromContainer(container, credentials, configPath, error);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to get the wg conf file from the server";
        return error;
    }

    auto client = clientsTable.at(row).toObject();
    QString clientId = client.value(configKey::clientId).toString();

    auto configSections = wireguardConfigString.split("[", Qt::SkipEmptyParts);
    for (auto &section : configSections) {
        if (section.contains(clientId)) {
            configSections.removeOne(section);
            break;
        }
    }
    QString newWireGuardConfig = configSections.join("[");
    newWireGuardConfig.insert(0, "[");
    error = sshSession->uploadTextFileToContainer(container, credentials, newWireGuardConfig, configPath);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to upload the wg conf file to the server";
        return error;
    }

    clientsTable.removeAt(row);

    const QByteArray clientsTableString = QJsonDocument(clientsTable).toJson();

    const QString clientsTableFile = QString("/opt/amnezia/%1/clientsTable")
                                         .arg(ContainerUtils::containerTypeToString(container));
    error = sshSession->uploadTextFileToContainer(container, credentials, clientsTableString, clientsTableFile);
    if (error != ErrorCode::NoError) {
        logger.error() << "Failed to upload the clientsTable file to the server";
        return error;
    }

    bool isAwg2 = (container == DockerContainer::Awg2);
    QString command = isAwg2 ? QStringLiteral("awg") : QStringLiteral("wg");
    QString iface   = isAwg2 ? QStringLiteral("awg0") : QStringLiteral("wg0");
    QString script  = QString(
        "sudo docker exec -i $CONTAINER_NAME bash -c '%1 syncconf %2 <(%1-quick strip %3)'"
    ).arg(command, iface, configPath);
    error = sshSession->runScript(
        credentials,
        sshSession->replaceVars(script, amnezia::genBaseVars(credentials, container, QString(), QString()))
    );
    if (error != ErrorCode::NoError) {
        logger.error() << QString("Failed to execute command '%1 syncconf %2' on the server").arg(command, iface);
        return error;
    }

    return ErrorCode::NoError;
}

ErrorCode UsersController::revokeClient(const QString &serverId, const int index, const DockerContainer container)
{
    if (index < 0 || index >= m_clientsTable.size()) {
        return ErrorCode::InternalError;
    }

    SshSession sshSession;
    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        return ErrorCode::InternalError;
    }
    ServerCredentials credentials = adminConfig->credentials();
    if (!credentials.isValid()) {
        return ErrorCode::InternalError;
    }

    QString clientId = m_clientsTable.at(index).toObject().value(configKey::clientId).toString();
    ErrorCode errorCode = ErrorCode::NoError;

    switch(container)
    {
        case DockerContainer::WireGuard:
        case DockerContainer::Awg:
        case DockerContainer::Awg2: {
            errorCode = revokeWireGuard(index, container, credentials, &sshSession, m_clientsTable);
            break;
        }
        default: {
            logger.error() << "Internal error: received unexpected container type";
            return ErrorCode::InternalError;
        }
    }

    if (errorCode == ErrorCode::NoError) {
        auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
        if (!adminConfig.has_value()) {
            return ErrorCode::InternalError;
        }
        ContainerConfig containerCfg = adminConfig->containerConfig(container);
        QString containerClientId = containerCfg.protocolConfig.clientId();

        const bool isAdminMatch = !clientId.isEmpty() && !containerClientId.isEmpty() && containerClientId.contains(clientId);
        if (isAdminMatch) {
            emit adminConfigRevoked(serverId, container);
        }

        emit clientRevoked(index);
    }

    emit clientsUpdated(m_clientsTable);
    emit revokeFinished(errorCode);

    return errorCode;
}

ErrorCode UsersController::revokeClient(const QString &serverId, const ContainerConfig &containerConfig, const DockerContainer container)
{
    SshSession sshSession;
    auto adminConfig = m_serversRepository->selfHostedAdminConfig(serverId);
    if (!adminConfig.has_value()) {
        return ErrorCode::InternalError;
    }
    ServerCredentials credentials = adminConfig->credentials();
    if (!credentials.isValid()) {
        return ErrorCode::InternalError;
    }

    ErrorCode errorCode = ErrorCode::NoError;
    errorCode = updateClients(serverId, container);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    switch(container)
    {
        case DockerContainer::WireGuard:
        case DockerContainer::Awg:
        case DockerContainer::Awg2: {
            break;
        }
        default: {
            logger.error() << "Internal error: received unexpected container type";
            return ErrorCode::InternalError;
        }
    }

    QString clientId = containerConfig.protocolConfig.clientId();

    int row = clientIndexById(clientId, m_clientsTable);
    if (row < 0) {
        return errorCode;
    }

    switch (container)
    {
    case DockerContainer::WireGuard:
    case DockerContainer::Awg:
    case DockerContainer::Awg2: {
        errorCode = revokeWireGuard(row, container, credentials, &sshSession, m_clientsTable);
        break;
    }
    default:
        logger.error() << "Internal error: received unexpected container type";
        return ErrorCode::InternalError;
    }

    if (errorCode == ErrorCode::NoError) {
        emit adminConfigRevoked(serverId, container);
        emit clientRevoked(row);
        emit clientsUpdated(m_clientsTable);
    }

    return errorCode;
}

