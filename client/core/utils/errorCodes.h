#ifndef ERRORCODES_H
#define ERRORCODES_H

#include <QMetaEnum>
#include <QObject>

namespace caelispect
{
    namespace error_code_ns
    {
      Q_NAMESPACE
      // TODO: change to enum class
      enum ErrorCode {
        // General error codes
        NoError = 0,
        UnknownError = 100,
        InternalError = 101,
        NotImplementedError = 102,
        CaelispectServiceNotRunning = 103,
        NotSupportedOnThisPlatform = 104,

        // Import errors
        ImportInvalidConfigError = 900,
        ImportOpenConfigError = 901,
        ImportBackupFileUseRestoreInstead = 903,
        RestoreBackupInvalidError = 904,

        // Android errors
        AndroidError = 1000,

        // Api errors
        ApiConfigDownloadError = 1100,
        ApiConfigTimeoutError = 1103,
        ApiConfigSslError = 1104,
        ApiMissingAgwPublicKey = 1105,
        ApiConfigDecryptionError = 1106,
        ApiNotFoundError = 1109,
        ApiUpdateRequestError = 1111,
        ApiRateLimitError = 1120,

        // QFile errors
        OpenError = 1200,
        ReadError = 1201,
        PermissionsError = 1202,
        UnspecifiedError = 1203,
        FatalError = 1204,
        AbortError = 1205
      };
      Q_ENUM_NS(ErrorCode)
    }

    using ErrorCode = error_code_ns::ErrorCode;
}

Q_DECLARE_METATYPE(caelispect::ErrorCode)

#endif // ERRORCODES_H
