#ifndef GM_SPA_CODEC_H
#define GM_SPA_CODEC_H

#include <mutex>

#include "spa/spaCodec.h"
#include "spa/spaConfig.h"
#include "spa/spaDeviceIdentity.h"

namespace spa {

class GmSpaCodec final : public Codec
{
public:
    GmSpaCodec(ProtocolConfig protocolConfig, DeviceIdentity deviceIdentity, QString appVersion);
    ~GmSpaCodec() override;

    static bool selfTest(QString *errorMessage = nullptr);

    EncodeResult encodeRequest(const Request &request) override;
    DecodeResult decodeResponse(const QByteArray &datagram, const Request &request) override;

private:
    void clearRequestState();

    ProtocolConfig m_protocolConfig;
    DeviceIdentity m_deviceIdentity;
    QString m_appVersion;
    QByteArray m_activeRequestId;
    QByteArray m_responseKey;
    std::mutex m_mutex;
};

} // namespace spa

#endif // GM_SPA_CODEC_H
