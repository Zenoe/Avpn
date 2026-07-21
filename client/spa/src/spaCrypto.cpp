#include "spaCrypto.h"

#include <openssl/evp.h>

namespace spa::crypto {

QByteArray sm3(const QByteArray &input)
{
    QByteArray result(EVP_MD_size(EVP_sm3()), Qt::Uninitialized);
    unsigned int resultLength = 0;
    if (EVP_Digest(input.constData(), static_cast<size_t>(input.size()),
                   reinterpret_cast<unsigned char *>(result.data()), &resultLength,
                   EVP_sm3(), nullptr) != 1) {
        return {};
    }
    result.resize(static_cast<qsizetype>(resultLength));
    return result;
}

} // namespace spa::crypto
