#include "caelispectApplication.h"

#include <QClipboard>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMimeData>
#include <QQuickItem>
#include <QQuickStyle>
#include <QResource>
#include <QStandardPaths>
#include <QTextDocument>
#include <QTimer>
#include <QTranslator>
#include <QEvent>
#include <QDir>
#include <QSettings>
#include <QtQuick/QQuickWindow>  
#include <QWindow>     

#include "core/protocols/qmlRegisterProtocols.h"
#include "logger.h"
#include "ui/controllers/qml/pageController.h"
#include "ui/models/installedAppsModel.h"
#include "ui/utils/mtProxyPublicHostInput.h"
#include "version.h"

#include "platforms/ios/QRCodeReaderBase.h"
         

bool CaelispectApplication::m_forceQuit = false;

CaelispectApplication::CaelispectApplication(int &argc, char *argv[]) : CAELISPECT_BASE_CLASS(argc, argv),
      m_optAutostart({QStringLiteral("a"), QStringLiteral("autostart")}, QStringLiteral("System autostart")),
      m_optCleanup  ({QStringLiteral("c"), QStringLiteral("cleanup")}, QStringLiteral("Cleanup logs")),
      m_optConnect  ({QStringLiteral("connect")}, QStringLiteral("Connect to server by index on startup"), QStringLiteral("index")),
      m_optImport   ({QStringLiteral("import")}, QStringLiteral("Import configuration from data string"), QStringLiteral("data"))
{
    setDesktopFileName(QStringLiteral(APPLICATION_NAME));
    setQuitOnLastWindowClosed(false);

    // Fix config file permissions
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    {
        QSettings s(ORGANIZATION_NAME, APPLICATION_NAME);
        s.setValue("permFixed", true);
    }

    QString configLoc1 = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation).first() + "/" + ORGANIZATION_NAME + "/"
            + APPLICATION_NAME + ".conf";
    QFile::setPermissions(configLoc1, QFileDevice::ReadOwner | QFileDevice::WriteOwner);

    QString configLoc2 = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation).first() + "/" + ORGANIZATION_NAME + "/"
            + APPLICATION_NAME + "/" + APPLICATION_NAME + ".conf";
    QFile::setPermissions(configLoc2, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
#endif

    m_settings = new SecureQSettings(ORGANIZATION_NAME, APPLICATION_NAME, this);
    m_nam = new QNetworkAccessManager(this);
}

CaelispectApplication::~CaelispectApplication()
{
#ifdef CAELISPECT_DESKTOP
    if (m_vpnConnection && m_vpnConnectionThread.isRunning()) {
        QMetaObject::invokeMethod(m_vpnConnection.get(), "shutdown", Qt::BlockingQueuedConnection);
    }
#endif

    if (m_engine) {
        delete m_engine;
        m_engine = nullptr;
    }

#ifdef CAELISPECT_DESKTOP
    m_spaUiController.reset();
#endif
    m_coreController.reset();
    m_vpnConnection.reset();

    m_vpnConnectionThread.requestInterruption();
    m_vpnConnectionThread.quit();

    if (!m_vpnConnectionThread.wait(3000)) {
        m_vpnConnectionThread.terminate();
        m_vpnConnectionThread.wait(500);
    }

}

#ifdef Q_OS_ANDROID
namespace {
    static void clearQtCaches()
    {
        const QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        if (!cacheRoot.isEmpty()) {
            QDir(cacheRoot + "/QtShaderCache").removeRecursively();
            QDir(cacheRoot + "/qmlcache").removeRecursively();
        }
    }
}
#endif

void CaelispectApplication::init()
{
    m_engine = new QQmlApplicationEngine;

    const QUrl url(QStringLiteral("qrc:/ui/qml/main2.qml"));
    QObject::connect(
        m_engine, &QQmlApplicationEngine::objectCreated, this,
        [this, url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                QCoreApplication::exit(-1);
                return;
            }
            // install filter on main window
            if (auto win = qobject_cast<QQuickWindow*>(obj)) {
                win->installEventFilter(this);
#ifdef Q_OS_ANDROID
                QObject::connect(win, &QQuickWindow::sceneGraphError,
                    [](QQuickWindow::SceneGraphError, const QString &msg) {
                        qWarning() << "Scene graph error (suppressed):" << msg;
                    });
                // Keep graphics context alive across hide/show cycles to avoid
                // eglSwapBuffers/makeCurrent being called on a context Android has reclaimed.
                win->setPersistentSceneGraph(true);
                win->setPersistentGraphics(true);
#endif
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
                win->show();
#elif defined(CAELISPECT_DESKTOP)
                win->show();
#else
                if (!m_coreController || !m_coreController->pageController()->shouldStartMinimized()) {
                    win->show();
                }
#endif
            }
        },
        Qt::QueuedConnection);

    m_engine->rootContext()->setContextProperty("Debug", &Logger::Instance());

#ifdef MACOS_NE
    m_engine->rootContext()->setContextProperty("IsMacOsNeBuild", true);
#else
    m_engine->rootContext()->setContextProperty("IsMacOsNeBuild", false);
#endif

    m_vpnConnection = QSharedPointer<VpnConnection>(new VpnConnection(nullptr, nullptr),
                                                     [](VpnConnection *connection) { connection->deleteLater(); });
    m_coreController.reset(new CoreController(m_vpnConnection, m_settings, m_engine));

    m_vpnConnection->moveToThread(&m_vpnConnectionThread);
    m_vpnConnectionThread.start();

#ifdef CAELISPECT_DESKTOP
    m_spaUiController.reset(new spa::UiController(QStringLiteral(APP_VERSION), this));
    m_engine->rootContext()->setContextProperty("SpaController", m_spaUiController.get());
#endif

    m_engine->addImportPath("qrc:/ui/qml/Modules/");

    if (m_parser.isSet(m_optImport)) {
        const QString data = m_parser.value(m_optImport);
        if (!data.isEmpty()) {
            if (m_coreController) {
                m_coreController->importConfigFromData(data);
            }
        }
    }

    m_engine->load(url);

    m_coreController->setQmlRoot();

#ifdef CAELISPECT_DESKTOP
    emit m_coreController->pageController()->raiseMainWindow();
#else
    m_coreController->pageController()->showOnStartup();
#endif

// Android TextArea clipboard workaround
// Text from TextArea always has "text/html" mime-type:
// /qt/6.6.1/Src/qtdeclarative/src/quick/items/qquicktextcontrol.cpp:1865
// Next, html is created for this mime-type:
// /qt/6.6.1/Src/qtdeclarative/src/quick/items/qquicktextcontrol.cpp:1885
// And this html goes to the Androids clipboard, i.e. text from TextArea is always copied as richText:
// /qt/6.6.1/Src/qtbase/src/plugins/platforms/android/androidjniclipboard.cpp:46
// So we catch all the copies to the clipboard and clear them from "text/html"
#ifdef Q_OS_ANDROID
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, []() {
        auto clipboard = QGuiApplication::clipboard();
        if (clipboard->mimeData()->hasHtml()) {
            clipboard->setText(clipboard->text());
        }
    });
#endif

    if (m_parser.isSet(m_optConnect)) {
        bool ok = false;
        int idx = m_parser.value(m_optConnect).toInt(&ok);
        if (ok) {
            QTimer::singleShot(0, this, [this, idx]() {
                if (m_coreController) {
                    m_coreController->openConnectionByIndex(idx);
                }
            });
        }
    }
}

void CaelispectApplication::registerTypes()
{
    qRegisterMetaType<ServerCredentials>("ServerCredentials");

    qRegisterMetaType<DockerContainer>("DockerContainer");
    using namespace caelispect::ProtocolEnumNS;
    qRegisterMetaType<TransportProto>("TransportProto");
    qRegisterMetaType<Proto>("Proto");
    qRegisterMetaType<ServiceType>("ServiceType");

    qmlRegisterType<QRCodeReader>("QRCodeReader", 1, 0, "QRCodeReader");

    m_containerProps.reset(new ContainerProps());
    qmlRegisterSingletonInstance("ContainerProps", 1, 0, "ContainerProps", m_containerProps.get());

    m_protocolProps.reset(new ProtocolProps());
    qmlRegisterSingletonInstance("ProtocolProps", 1, 0, "ProtocolProps", m_protocolProps.get());

    qmlRegisterSingletonType(QUrl("qrc:/ui/qml/Filters/ContainersModelFilters.qml"), "ContainersModelFilters", 1, 0,
                             "ContainersModelFilters");

    qmlRegisterType<InstalledAppsModel>("InstalledAppsModel", 1, 0, "InstalledAppsModel");

    qmlRegisterType<PublicHostInputValidator>("MtProxyConfig", 1, 0, "PublicHostInputValidator");
    qmlRegisterType<PublicHostInputValidator>("TelemtConfig", 1, 0, "PublicHostInputValidator");

    caelispect::declareQmlProtocolEnum();
    Vpn::declareQmlVpnConnectionStateEnum();
    PageLoader::declareQmlPageEnum();
}

void CaelispectApplication::loadFonts()
{
    QQuickStyle::setStyle("Basic");
}

bool CaelispectApplication::parseCommands()
{
    m_parser.setApplicationDescription(APPLICATION_NAME);
    m_parser.addHelpOption();
    m_parser.addVersionOption();

    m_parser.addOption(m_optAutostart);
    m_parser.addOption(m_optCleanup);
    
    m_parser.process(*this);

    if (m_parser.isSet(m_optCleanup)) {
        Logger::cleanUp();
        QTimer::singleShot(100, this, [this] { quit(); });
        exec();
        return false;
    }
    return true;
}

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
void CaelispectApplication::startLocalServer() {
    const QString serverName("CaelispectInstance");
    QLocalServer::removeServer(serverName);

    QLocalServer *server = new QLocalServer(this);
    server->listen(serverName);

    QObject::connect(server, &QLocalServer::newConnection, this, [server, this]() {
        if (server) {
            QLocalSocket *clientConnection = server->nextPendingConnection();
            clientConnection->deleteLater();
        }
        emit m_coreController->pageController()->raiseMainWindow(); //TODO
    });
}
#endif

bool CaelispectApplication::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Close) {
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        quit();
#else
        if (m_forceQuit) {
            quit();
        } else {
            if (m_coreController && m_coreController->pageController()) {
                m_coreController->pageController()->hideMainWindow();
            }
        }
#endif
        return true; // eat the close
    }
    // call base QObject::eventFilter
    return QObject::eventFilter(watched, event);
}

void CaelispectApplication::forceQuit()
{
    m_forceQuit = true;
    quit();
}

QQmlApplicationEngine *CaelispectApplication::qmlEngine() const
{
    return m_engine;
}

QNetworkAccessManager *CaelispectApplication::networkManager()
{
    return m_nam;
}

QClipboard *CaelispectApplication::getClipboard()
{
    return this->clipboard();
}
