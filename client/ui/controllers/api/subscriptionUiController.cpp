#include "subscriptionUiController.h"

#include "amneziaApplication.h"
#include "core/configurators/wireguardConfigurator.h"
#include "core/utils/serverConfigUtils.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/api/apiUtils.h"
#include "core/utils/qrCodeUtils.h"
#include "ui/controllers/systemController.h"
#include "version.h"
#include <QClipboard>
#include <QDebug>
#include <QSet>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QTimer>

namespace
{
    namespace configKey
    {
        constexpr char awg[] = "awg";

        constexpr char apiEndpoint[] = "api_endpoint";
        constexpr char accessToken[] = "api_key";
        constexpr char certificate[] = "certificate";
        constexpr char publicKey[] = "public_key";
        constexpr char protocol[] = "protocol";

        constexpr char uuid[] = "installation_uuid";
        constexpr char osVersion[] = "os_version";
        constexpr char appVersion[] = "app_version";

        constexpr char userCountryCode[] = "user_country_code";
        constexpr char serverCountryCode[] = "server_country_code";
        constexpr char serviceType[] = "service_type";
        constexpr char serviceInfo[] = "service_info";
        constexpr char serviceProtocol[] = "service_protocol";

        constexpr char apiPayload[] = "api_payload";
        constexpr char keyPayload[] = "key_payload";

        constexpr char apiConfig[] = "api_config";
        constexpr char authData[] = "auth_data";

        constexpr char config[] = "config";

        constexpr char subscription[] = "subscription";
        constexpr char endDate[] = "end_date";

        constexpr char isConnectEvent[] = "is_connect_event";
    }

}

SubscriptionUiController::SubscriptionUiController(ServersController* serversController,
                                           SubscriptionController* subscriptionController,
                                           ApiAccountInfoModel* apiAccountInfoModel,
                                           ApiCountryModel* apiCountryModel,
                                           ApiDevicesModel* apiDevicesModel,
                                           SettingsController* settingsController,
                                           ConnectionController* connectionController,
                                           QObject *parent)
    : QObject(parent),
      m_serversController(serversController),
      m_subscriptionController(subscriptionController),
      m_apiAccountInfoModel(apiAccountInfoModel),
      m_apiCountryModel(apiCountryModel),
      m_apiDevicesModel(apiDevicesModel),
      m_settingsController(settingsController),
      m_connectionController(connectionController)
{
    connect(this, &SubscriptionUiController::installServerFromApiFinished, this,
            [this](const QString &, int preferredDefaultServerIndex) {
        if (m_connectionController->isConnected()) {
            return;
        }

        const int selectedServerIndex = preferredDefaultServerIndex >= 0
                ? preferredDefaultServerIndex
                : (m_serversController->getServersCount() - 1);
        const QString serverId = m_serversController->getServerId(selectedServerIndex);
        if (!serverId.isEmpty()) {
            m_serversController->setDefaultServer(serverId);
        }
    });
}

bool SubscriptionUiController::exportVpnKey(const QString &serverId, const QString &fileName)
{
    if (fileName.isEmpty()) {
        emit errorOccurred(ErrorCode::PermissionsError);
        return false;
    }

    prepareVpnKeyExport(serverId);
    if (m_vpnKey.isEmpty()) {
        emit errorOccurred(ErrorCode::ApiConfigEmptyError);
        return false;
    }

    return SystemController::saveFile(fileName, m_vpnKey);
}


bool SubscriptionUiController::exportNativeConfig(const QString &serverId, const QString &serverCountryCode, const QString &fileName)
{
    if (fileName.isEmpty()) {
        emit errorOccurred(ErrorCode::PermissionsError);
        return false;
    }

    QString nativeConfig;
    ErrorCode errorCode = m_subscriptionController->exportNativeConfig(serverId, serverCountryCode, nativeConfig);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    const bool saved = SystemController::saveFile(fileName, nativeConfig);
    getAccountInfo(serverId, true);
    return saved;
}


bool SubscriptionUiController::revokeNativeConfig(const QString &serverId, const QString &serverCountryCode)
{
    ErrorCode errorCode = m_subscriptionController->revokeNativeConfig(serverId, serverCountryCode);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }
    return true;
}


void SubscriptionUiController::prepareVpnKeyExport(const QString &serverId)
{
    QString vpnKey;
    ErrorCode errorCode = m_subscriptionController->prepareVpnKeyExport(serverId, vpnKey);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return;
    }

    m_vpnKey = vpnKey;

    QString vpnKeyForQr = vpnKey;
    vpnKeyForQr.replace("vpn://", "");

    m_qrCodes = qrCodeUtils::generateQrCodeImageSeries(vpnKeyForQr.toUtf8());

    emit vpnKeyExportReady();
}


void SubscriptionUiController::copyVpnKeyToClipboard()
{
    auto clipboard = amnApp->getClipboard();
    clipboard->setText(m_vpnKey);
}

bool SubscriptionUiController::updateServiceFromGateway(const QString &serverId, const QString &newCountryCode, const QString &newCountryName,
                                                    bool reloadServiceConfig)
{
    bool isConnectEvent = newCountryCode.isEmpty() && newCountryName.isEmpty() && !reloadServiceConfig;
    bool wasSubscriptionExpired = false;
    if (const auto oldApiV2 = m_serversController->apiV2Config(serverId)) {
        wasSubscriptionExpired = oldApiV2->apiConfig.subscriptionExpiredByServer
                || oldApiV2->apiConfig.isSubscriptionExpired();
    }

    ErrorCode errorCode = m_subscriptionController->updateServiceFromGateway(serverId, newCountryCode, isConnectEvent);

    if (errorCode == ErrorCode::NoError) {
        if (wasSubscriptionExpired) {
            emit subscriptionRefreshNeeded();
        }
        if (reloadServiceConfig) {
            emit reloadServerFromApiFinished(tr("API config reloaded"));
        } else if (newCountryName.isEmpty()) {
            emit updateServerFromApiFinished();
        } else {
            emit changeApiCountryFinished(tr("Successfully changed the country of connection to %1").arg(newCountryName));
        }
        return true;
    } else {
        if (errorCode == ErrorCode::ApiSubscriptionExpiredError) {
            emit subscriptionExpiredOnServer();
        } else {
            emit errorOccurred(errorCode);
        }
        return false;
    }
}


bool SubscriptionUiController::deactivateDevice(const QString &serverId)
{
    ErrorCode errorCode = m_subscriptionController->deactivateDevice(serverId);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    return true;
}


bool SubscriptionUiController::deactivateExternalDevice(const QString &serverId, const QString &uuid, const QString &serverCountryCode)
{
    ErrorCode errorCode = m_subscriptionController->deactivateExternalDevice(serverId, uuid, serverCountryCode);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    return true;
}


void SubscriptionUiController::validateConfig()
{
    const QString serverId = m_serversController->getDefaultServerId();
    if (serverId.isEmpty()) {
        emit configValidated(false);
        return;
    }

    bool hasInstalledContainers = m_serversController->hasInstalledContainers(serverId);

    ErrorCode errorCode = m_subscriptionController->validateAndUpdateConfig(serverId, hasInstalledContainers);

    if (errorCode != ErrorCode::NoError) {
        if (errorCode == ErrorCode::ApiSubscriptionExpiredError) {
            emit subscriptionExpiredOnServer();
        } else {
            emit errorOccurred(errorCode);
        }
        emit configValidated(false);
        return;
    }
    emit configValidated(true);
}

void SubscriptionUiController::setCurrentProtocol(const QString &serverId, const QString &protocolName)
{
    m_subscriptionController->setCurrentProtocol(serverId, protocolName);
}



QString SubscriptionUiController::currentProtocol(const QString &serverId)
{
    return m_subscriptionController->currentProtocol(serverId);
}


QStringList SubscriptionUiController::availableProtocols(const QString &serverId)
{
    return m_subscriptionController->availableProtocols(serverId);
}


void SubscriptionUiController::removeApiConfig(const QString &serverId)
{
    m_subscriptionController->removeApiConfig(serverId);
    emit apiConfigRemoved(tr("Api config removed"));
}

void SubscriptionUiController::removeServer(const QString &serverId)
{
    const QString serverName = m_serversController->notificationDisplayName(serverId);
    if (!m_subscriptionController->removeServer(serverId)) {
        return;
    }
    emit apiServerRemoved(tr("Server '%1' was removed").arg(serverName));
}


QList<QString> SubscriptionUiController::getQrCodes()
{
    return m_qrCodes;
}

int SubscriptionUiController::getQrCodesCount()
{
    return static_cast<int>(m_qrCodes.size());
}

QString SubscriptionUiController::getVpnKey()
{
    return m_vpnKey;
}

bool SubscriptionUiController::getAccountInfo(const QString &serverId, bool reload)
{
    if (reload) {
        QEventLoop wait;
        QTimer::singleShot(1000, &wait, &QEventLoop::quit);
        wait.exec(QEventLoop::ExcludeUserInputEvents);
    }
    QJsonObject accountInfo;
    ErrorCode errorCode = m_subscriptionController->getAccountInfo(serverId, accountInfo);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    const auto apiV2 = m_serversController->apiV2Config(serverId);
    if (!apiV2.has_value()) {
        emit errorOccurred(ErrorCode::InternalError);
        return false;
    }
    m_apiAccountInfoModel->updateModel(accountInfo, apiV2->toJson());

    if (reload) {
        updateApiCountryModel();
        updateApiDevicesModel();
    }

    return true;
}

void SubscriptionUiController::updateApiCountryModel()
{
    m_apiCountryModel->updateModel(m_apiAccountInfoModel->getAvailableCountries(), "");
    m_apiCountryModel->updateIssuedConfigsInfo(m_apiAccountInfoModel->getIssuedConfigsInfo());
}

void SubscriptionUiController::updateApiDevicesModel()
{
    m_apiDevicesModel->updateModel(m_apiAccountInfoModel->getIssuedConfigsInfo(), m_settingsController->getInstallationUuid(false));
}

void SubscriptionUiController::getRenewalLink(const QString &serverId)
{
    if (serverId.isEmpty()) {
        emit errorOccurred(ErrorCode::InternalError);
        return;
    }

    auto *watcher = new QFutureWatcher<QPair<ErrorCode, QString>>(this);
    connect(watcher, &QFutureWatcher<QPair<ErrorCode, QString>>::finished, this, [this, watcher]() {
        const auto [errorCode, url] = watcher->result();
        watcher->deleteLater();
        if (errorCode != ErrorCode::NoError) {
            emit errorOccurred(errorCode);
            return;
        }
        if (url.isEmpty()) {
            return;
        }
        emit renewalLinkReceived(url);
    });
    watcher->setFuture(m_subscriptionController->getRenewalLink(serverId));
}

