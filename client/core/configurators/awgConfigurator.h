#ifndef AWGCONFIGURATOR_H
#define AWGCONFIGURATOR_H

#include <QObject>

#include "wireguardConfigurator.h"

class AwgConfigurator : public WireguardConfigurator
{
    Q_OBJECT
public:
    explicit AwgConfigurator(QObject *parent = nullptr);
};

#endif // AWGCONFIGURATOR_H
