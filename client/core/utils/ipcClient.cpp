#include "ipcClient.h"
#include "ipc.h"
#include <QRemoteObjectNode>
#include <QtNetwork/qlocalsocket.h>

#include <memory>

namespace {
thread_local std::unique_ptr<IpcClient> ipcClient;
}

IpcClient::IpcClient(QObject *parent) : QObject(parent)
{
    m_node.connectToNode(QUrl("local:" + caelispect::getIpcServiceUrl()));
    m_interface.reset(m_node.acquire<IpcInterfaceReplica>());
}

IpcClient& IpcClient::Instance()
{
    if (!ipcClient) {
        ipcClient = std::make_unique<IpcClient>();
    }
    return *ipcClient;
}

void IpcClient::deinit()
{
    ipcClient.reset();
}

QSharedPointer<IpcInterfaceReplica> IpcClient::Interface()
{
    QSharedPointer<IpcInterfaceReplica> rep = Instance().m_interface;
    if (rep.isNull()) {
        qCritical() << "IpcClient::Interface(): Failed to acquire replica";
        return nullptr;
    }
    if (!rep->waitForSource(1000)) {
        qCritical() << "IpcClient::Interface(): Failed to initialize replica";
        return nullptr;
    }
    if (!rep->isReplicaValid()) {
        qWarning() << "IpcClient::Interface(): Replica is invalid";
    }
    return rep;
}

QSharedPointer<IpcProcessInterfaceReplica> IpcClient::CreatePrivilegedProcess()
{
    return withInterface([](QSharedPointer<IpcInterfaceReplica> &iface) -> QSharedPointer<IpcProcessInterfaceReplica> {
        auto createPrivilegedProcess = iface->createPrivilegedProcess();
        if (!createPrivilegedProcess.waitForFinished()) {
            qCritical() << "Failed to create privileged process";
            return nullptr;
        }

        const int pid = createPrivilegedProcess.returnValue();

        auto* node = new QRemoteObjectNode();
        node->connectToNode(QUrl(QString("local:%1").arg(caelispect::getIpcProcessUrl(pid))));

        QSharedPointer<IpcProcessInterfaceReplica> rep(
            node->acquire<IpcProcessInterfaceReplica>(),
            [node] (IpcProcessInterfaceReplica *ptr) {
                delete ptr;
                node->deleteLater();
            }
        );
        if (rep.isNull()) {
            qCritical() << "IpcClient::CreatePrivilegedProcess(): Failed to acquire replica";
            return nullptr;
        }
        if (!rep->waitForSource()) {
            qCritical() << "IpcClient::CreatePrivilegedProcess(): Failed to initialize replica";
            return nullptr;
        }
        if (!rep->isReplicaValid()) {
            qCritical() << "IpcClient::CreatePrivilegedProcess(): Replica is invalid";
            return nullptr;
        }

        return rep;
    },
    []() -> QSharedPointer<IpcProcessInterfaceReplica> {
        return nullptr;
    });
}
