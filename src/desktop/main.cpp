#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include <memory>
#include <iostream>

#include "core/application/ApplicationBootstrap.hpp"
#include "presentation/AppViewModel.hpp"
#include "presentation/CaseDashboardViewModel.hpp"
#include "presentation/OperationManagerViewModel.hpp"

using namespace fvm::core::application;
using namespace fvm::presentation;

int main(int argc, char *argv[])
{
    // Enable high-DPI scaling
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);
    app.setApplicationName("ForensicVM Desktop");
    app.setOrganizationName("ForensicVM");

    // Instantiate Forensic Core
    std::shared_ptr<fvm::core::application::contracts::IForensicApplication> forensicApp;
    try {
        forensicApp = ApplicationBootstrap::createApplication();
    } catch (const std::exception& e) {
        qFatal("Failed to initialize Forensic Core: %s", e.what());
        return -1;
    }

    // Wrap in ViewModels
    AppViewModel appViewModel(forensicApp);

    // Ensure safe shutdown of forensic application when GUI is closed
    QObject::connect(&app, &QGuiApplication::aboutToQuit, [&appViewModel]() {
        qDebug() << "Application shutting down, propagating cancellation to active operations...";
        // Close case and trigger destruction of AppViewModel which will invoke backend shutdown 
        // safely through the IForensicApplication destructor as built in Phase 5.
        appViewModel.closeCase();
    });

    QQmlApplicationEngine engine;

    // Expose ViewModels to QML
    engine.rootContext()->setContextProperty("appViewModel", &appViewModel);
    engine.rootContext()->setContextProperty("dashboardViewModel", appViewModel.dashboard());
    engine.rootContext()->setContextProperty("operationManager", appViewModel.dashboard()->operationManager());

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
