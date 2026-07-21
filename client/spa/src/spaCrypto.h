#ifndef SPA_CRYPTO_H
#define SPA_CRYPTO_H

#include <QByteArray>

namespace spa::crypto {

QByteArray sm3(const QByteArray &input);

} // namespace spa::crypto

#endif // SPA_CRYPTO_H
