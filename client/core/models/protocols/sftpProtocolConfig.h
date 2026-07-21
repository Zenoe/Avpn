#ifndef SFTPPROTOCOLCONFIG_H
#define SFTPPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>

namespace caelispect
{

struct SftpProtocolConfig {
    QString port;
    QString userName;
    QString password;
    
    QJsonObject toJson() const;
    static SftpProtocolConfig fromJson(const QJsonObject& json);
};

} // namespace caelispect

#endif // SFTPPROTOCOLCONFIG_H

