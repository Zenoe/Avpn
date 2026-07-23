#ifndef WIREGUARDPROFILE_H
#define WIREGUARDPROFILE_H

#include <QJsonObject>
#include <QPair>
#include <QString>

#include "core/models/protocols/wireGuardProtocolConfig.h"

namespace caelispect
{

// A user-imported standard WireGuard configuration. One profile always means one tunnel.
struct WireGuardProfile
{
    QString description;
    QString hostName;
    QString dns1;
    QString dns2;
    WireGuardClientConfig config;

    QString displayName() const;
    QPair<QString, QString> dnsPair(const QString &fallbackPrimary, const QString &fallbackSecondary) const;
    QJsonObject toJson() const;
    static WireGuardProfile fromJson(const QJsonObject &json);
    static bool isValidJson(const QJsonObject &json);
};

} // namespace caelispect

#endif // WIREGUARDPROFILE_H
