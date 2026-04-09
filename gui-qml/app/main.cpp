#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QQuickStyle>

#include "../infrastructure/DictionaryRepository.h"
#include "../infrastructure/SearchService.h"
#include "../infrastructure/SrsService.h"

#include "../viewmodels/SearchResultModel.h"
#include "../viewmodels/SearchViewModel.h"
#include "../viewmodels/EntryDetailsViewModel.h"
#include "../viewmodels/SrsViewModel.h"
#include "../viewmodels/SrsLibraryViewModel.h"

#include "Configuration.h"
#include "AppPaths.h"
#include "AppController.h"

#include <stdlib.h>

int main(int argc, char **argv) {
    #ifdef Q_OS_ANDROID
    qputenv("QSG_RENDER_LOOP", "threaded");
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    #endif
    QGuiApplication app(argc, argv);
    app.setApplicationName("KotobaPlus");
    app.setOrganizationName("KotobaPlus");

    QQuickStyle::setStyle("Material");

    // ── ViewModels ────────────────────────────────────────────────────────────
    SearchResultModel     *searchModel = new SearchResultModel();
    SearchViewModel       *searchVM    = new SearchViewModel();
    EntryDetailsViewModel *detailsVM   = new EntryDetailsViewModel();
    SrsViewModel          *srsVM       = new SrsViewModel();
    SrsLibraryViewModel   *libVM       = new SrsLibraryViewModel();

    // ── AppController ─────────────────────────────────────────────────────────
    AppController *controller = new AppController();

    // ── QML engine ────────────────────────────────────────────────────────────
    QQmlApplicationEngine engine;
    ConfigWrapper configWrapper;

    engine.rootContext()->setContextProperty("searchVM",      searchVM);
    engine.rootContext()->setContextProperty("searchModel",   searchModel);
    engine.rootContext()->setContextProperty("detailsVM",     detailsVM);
    engine.rootContext()->setContextProperty("srsVM",         srsVM);
    engine.rootContext()->setContextProperty("appConfig",     &configWrapper);
    engine.rootContext()->setContextProperty("srsLibraryVM",  libVM);
    engine.rootContext()->setContextProperty("appController", controller);

    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) return -1;

    // ── Servicios pesados (Search/SRS) ────────────────────────────────────────
    SearchService *searchSvc = new SearchService();
    SrsService    *srsSvc    = new SrsService();
    srand(static_cast<unsigned int>(time(nullptr)));

    std::string srsProfilePath;
    bool failed = false;

    // ── Hilo concurrente para carga pesada ─────────────────────────────────────
    QThreadPool::globalInstance()->start([=, &configWrapper, &srsProfilePath, &failed]() {

        // ── 1) Paths y configuración ─────────────────────────────────────────
        AppPaths paths = AppPaths::resolve();

    #ifdef Q_OS_ANDROID
        AppPaths::extractAssetsIfNeeded(paths.dataDir);
    #endif

        loadConfiguration(configWrapper.m_config, paths.configPath);
        configWrapper.m_configPath = paths.configPath;

        configWrapper.m_config.dictPath      = paths.dictPath;
        configWrapper.m_config.dictIndexPath = paths.dictIndexPath;
        configWrapper.m_config.glossEnPath   = paths.glossPath("en");
        configWrapper.m_config.glossEsPath   = paths.glossPath("es");
        configWrapper.m_config.glossDePath   = paths.glossPath("de");
        configWrapper.m_config.glossFrPath   = paths.glossPath("fr");
        configWrapper.m_config.glossHuPath   = paths.glossPath("hu");
        configWrapper.m_config.glossNlPath   = paths.glossPath("nl");
        configWrapper.m_config.glossRuPath   = paths.glossPath("ru");
        configWrapper.m_config.glossSlvPath  = paths.glossPath("slv");
        configWrapper.m_config.glossSvPath   = paths.glossPath("sv");
        configWrapper.m_config.jpPath        = paths.glossPath("jp");

        srsProfilePath = configWrapper.m_config.srsPath.toStdString();

        // ── 2) Abrir diccionario ─────────────────────────────────────────────
        QMetaObject::invokeMethod(controller, [=]() {
            controller->setStatus("Opening dictionary…");
        }, Qt::QueuedConnection);

        DictionaryRepository *repo = new DictionaryRepository();
        if (!repo->open(configWrapper.m_config.dictPath,
                        configWrapper.m_config.dictIndexPath)) {
            qWarning("Failed to open dictionary.");
            failed = true;
            return;
        }
        kotoba_dict *dict = repo->dict();

        // ── 3) Inicializar servicios ───────────────────────────────────────
        QMetaObject::invokeMethod(controller, [=]() {
            controller->setStatus("Initializing search…");
        }, Qt::QueuedConnection);
        searchSvc->initialize(dict, &configWrapper.m_config);

        QMetaObject::invokeMethod(controller, [=]() {
            controller->setStatus("Loading SRS profile…");
        }, Qt::QueuedConnection);
        srsSvc->initialize(static_cast<uint32_t>(dict->bin_header->entry_count),
                        &configWrapper.m_config);
        srsSvc->load(srsProfilePath.c_str());

        // ── 4) Inicializar ViewModels en hilo principal ────────────────
        QMetaObject::invokeMethod(qApp, [=, &configWrapper]() {
            searchVM->initialize(searchSvc, searchModel, dict, &configWrapper.m_config);
            detailsVM->initialize(dict, &configWrapper.m_config);
            srsVM->initialize(srsSvc, dict, detailsVM);
            libVM->initialize(srsSvc, dict, searchSvc, &configWrapper.m_config);

            configWrapper.setServices(searchSvc, srsSvc);
            controller->notifyReady();  // dispara transición QML 
        }, Qt::QueuedConnection);

    });

    if (failed) {
        // Error crítico durante la carga inicial — mostrar mensaje y salir.
        return -1;
    }

    // ── Ciclo de vida Android ─────────────────────────────────────────────────
    #ifdef Q_OS_ANDROID
    QObject::connect(&app, &QGuiApplication::applicationStateChanged,
        [&](Qt::ApplicationState state) {

        switch (state) {

        case Qt::ApplicationSuspended:
            // App en background profundo — Android puede matarla en cualquier momento.
            // Guardamos inmediatamente sin esperar al aboutToQuit.
            srsSvc->save(srsProfilePath.c_str());
            saveConfiguration(configWrapper.m_config, configWrapper.m_configPath);
            break;

        case Qt::ApplicationHidden:
            // Minimizando — notificar a QML para que congele la UI.
            QMetaObject::invokeMethod(controller, [=]() {
                controller->setAppActive(false);
            }, Qt::QueuedConnection);
            break;

        case Qt::ApplicationInactive:
            // Foco perdido pero aún visible (notificación encima, etc.).
            // No hacemos nada agresivo.
            break;

        case Qt::ApplicationActive:
            // Volviendo al frente — reactivar UI y refrescar contadores SRS
            // (puede haber cambiado la fecha mientras estaba en background).
            QMetaObject::invokeMethod(controller, [=]() {
                controller->setAppActive(true);
            }, Qt::QueuedConnection);
            QMetaObject::invokeMethod(qApp, [=]() {
                srsVM->refresh();
            }, Qt::QueuedConnection);
            break;
        }
    });
    #endif

    // ── Persistir cambios al cerrar ─────────────────────────────────────────
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        srsSvc->save();
        saveConfiguration(configWrapper.m_config, configWrapper.m_configPath);
    });

    // ── Ejecutar app ───────────────────────────────────────────────────────
    int result = app.exec();

    // ── Limpiar memoria ───────────────────────────────────────────────────
    delete libVM;
    delete srsVM;
    delete detailsVM;
    delete searchVM;
    delete searchModel;
    delete controller;
    delete srsSvc;
    delete searchSvc;

    return result;
}