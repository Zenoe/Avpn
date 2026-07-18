#ifndef SPA_CODEC_H
#define SPA_CODEC_H

#include <QByteArray>
#include <QString>

#include "spa/spaTypes.h"

namespace spa {

struct EncodeResult
{
    QByteArray datagram;
    QString errorMessage;

    bool isSuccess() const
    {
        return !datagram.isEmpty() && errorMessage.isEmpty();
    }
};

// The codec is the security boundary of the SPA protocol. A production codec
// must authenticate and decrypt the response before returning Accepted.
class Codec
{
public:
    virtual ~Codec() = default;

    virtual EncodeResult encodeRequest(const Request &request) = 0;
    virtual DecodeResult decodeResponse(const QByteArray &datagram, const Request &request) = 0;
};

} // namespace spa

#endif // SPA_CODEC_H

