#include "errorStrings.h"

using namespace caelispect;

QString errorString(ErrorCode code) {
    QString errorMessage;

    switch (code) {

    // General error codes
    case(ErrorCode::NoError): errorMessage = QObject::tr("No error"); break;
    case(ErrorCode::UnknownError): errorMessage = QObject::tr("Unknown error"); break;
    case(ErrorCode::NotImplementedError): errorMessage = QObject::tr("Function not implemented"); break;
    case(ErrorCode::CaelispectServiceNotRunning): errorMessage = QObject::tr("Background service is not running"); break;
    case(ErrorCode::NotSupportedOnThisPlatform): errorMessage = QObject::tr("The selected protocol is not supported on the current platform"); break;

    case (ErrorCode::ImportInvalidConfigError): errorMessage = QObject::tr("The file is not a valid WireGuard configuration"); break;
    case (ErrorCode::ImportBackupFileUseRestoreInstead): errorMessage = QObject::tr("Backup files cannot be imported here. Use 'Restore from backup' instead."); break;
    case (ErrorCode::RestoreBackupInvalidError): errorMessage = QObject::tr("Backup file is corrupted or has invalid format"); break;
    case (ErrorCode::ImportOpenConfigError): errorMessage = QObject::tr("Unable to open config file"); break;

    // Android errors
    case (ErrorCode::AndroidError): errorMessage = QObject::tr("VPN connection error"); break;

    // Api errors
    case (ErrorCode::ApiConfigDownloadError): errorMessage = QObject::tr("Error when retrieving configuration from API"); break;
    case (ErrorCode::ApiConfigSslError): errorMessage = QObject::tr("SSL error occurred"); break;
    case (ErrorCode::ApiConfigTimeoutError): errorMessage = QObject::tr("Server response timeout on api request"); break;
    case (ErrorCode::ApiMissingAgwPublicKey): errorMessage = QObject::tr("Missing AGW public key"); break;
    case (ErrorCode::ApiConfigDecryptionError): errorMessage = QObject::tr("Failed to decrypt response payload"); break;
    case (ErrorCode::ApiNotFoundError): errorMessage = QObject::tr("Error when retrieving configuration from API"); break;
    case (ErrorCode::ApiUpdateRequestError): errorMessage = QObject::tr("Please update the application to use this feature"); break;
    case (ErrorCode::ApiRateLimitError): errorMessage = QObject::tr("Too many requests. Please try again later"); break;

    // QFile errors
    case(ErrorCode::OpenError): errorMessage = QObject::tr("QFile error: The file could not be opened"); break;
    case(ErrorCode::ReadError): errorMessage = QObject::tr("QFile error: An error occurred when reading from the file"); break;
    case(ErrorCode::PermissionsError): errorMessage = QObject::tr("QFile error: The file could not be accessed"); break;
    case(ErrorCode::UnspecifiedError): errorMessage =  QObject::tr("QFile error: An unspecified error occurred"); break;
    case(ErrorCode::FatalError): errorMessage =  QObject::tr("QFile error: A fatal error occurred"); break;
    case(ErrorCode::AbortError): errorMessage =  QObject::tr("QFile error: The operation was aborted"); break;

    case(ErrorCode::InternalError):
    default:
        errorMessage = QObject::tr("Internal error"); break;
    }

    return QObject::tr("ErrorCode: %1. ").arg(code) + errorMessage;
}

QDebug operator<<(QDebug debug, const ErrorCode &e)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "ErrorCode::" << int(e) << "(" << errorString(e) << ")";

    return debug;
}
