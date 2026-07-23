#ifndef AUTHENTICATION_PASSWORD_CRYPTO_H
#define AUTHENTICATION_PASSWORD_CRYPTO_H

#include <QString>

namespace authentication::crypto {

bool encryptPassword(const QString &password, QString *encryptedPassword, QString *errorMessage);

} // namespace authentication::crypto

#endif // AUTHENTICATION_PASSWORD_CRYPTO_H
