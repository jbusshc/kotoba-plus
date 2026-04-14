#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QThreadPool>
#include <QMetaObject>
#include <QDebug>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <QtCore/qnativeinterface.h>
#endif

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

int main(int argc, char **argv)
{
    #ifdef Q_OS_ANDROID
    qputenv("QSG_RENDER_LOOP", "basic");
    QSurfaceFormat fmt;
    fmt.setSwapInterval(0);  // 0 = no vsync
    QSurfaceFormat::setDefaultFormat(fmt);
    #endif

    QGuiApplication app(argc, argv);
    app.setApplicationName("KotobaPlus");
    app.setOrganizationName("KotobaPlus");

    QQuickStyle::setStyle("Material");

    // ── ViewModels ─────────────────────────────────────────────
    auto *searchModel = new SearchResultModel();
    auto *searchVM    = new SearchViewModel();
    auto *detailsVM   = new EntryDetailsViewModel();
    auto *srsVM       = new SrsViewModel();
    auto *libVM       = new SrsLibraryViewModel();
    auto *controller  = new AppController();

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
    if (engine.rootObjects().isEmpty())
        return -1;

    // ── Obtener la QQuickWindow ─────────────────────────────────
    QQuickWindow *window = nullptr;
    for (QObject *obj : engine.rootObjects()) {
        window = qobject_cast<QQuickWindow *>(obj);
        if (window) break;
    }

#ifdef Q_OS_ANDROID
    // ── Gestión del ciclo de vida desde C++ ─────────────────────
    //
    // Qt traduce onPause/onResume/onStop a applicationStateChanged.
    // Cuando el estado pasa a Suspended (onStop en Android), Qt destruye
    // QtSurface y el contexto EGL, causando el lag al volver.
    //
    // Con setPersistentGraphics/SceneGraph le decimos a Qt que NO libere
    // esos recursos. Hay que hacerlo ANTES de que la app entre en Suspended,
    // es decir, conectarse a applicationStateChanged y actuar en el momento.
    //
    // Estados Android → Qt:
    //   onResume  → Qt::ApplicationActive
    //   onPause   → Qt::ApplicationInactive
    //   onStop    → Qt::ApplicationSuspended  ← aquí Qt destruiría la surface
    //   onDestroy → señal aboutToQuit

    if (window) {
        // Activar persistencia inicial
        window->setPersistentGraphics(true);
        window->setPersistentSceneGraph(true);
        qDebug() << "KotobaPlus: persistent graphics activado";

        QObject::connect(&app, &QGuiApplication::applicationStateChanged,
                         window, [window](Qt::ApplicationState state) {

            switch (state) {
            case Qt::ApplicationSuspended:
                // App va a background (onStop). Reforzar persistencia
                // justo antes de que Qt procese el evento de suspensión.
                window->setPersistentGraphics(true);
                window->setPersistentSceneGraph(true);
                qDebug() << "KotobaPlus: Suspended — graphics persistentes reforzados";
                break;

            case Qt::ApplicationInactive:
                // onPause — la app pierde el foco pero sigue visible (ej: notificación)
                // No hacemos nada especial aquí.
                qDebug() << "KotobaPlus: Inactive (onPause)";
                break;

            case Qt::ApplicationActive:
                // onResume — volvemos al frente
                // Forzar un redraw por si Qt necesita sincronizar
                window->requestUpdate();
                qDebug() << "KotobaPlus: Active (onResume)";
                break;

            default:
                break;
            }
        });
    }
#endif

    // ── Servicios ──────────────────────────────────────────────
    auto *searchSvc = new SearchService();
    auto *srsSvc    = new SrsService();

    ConfigWrapper *cfg = &configWrapper;

    // ── Background thread ──────────────────────────────────────
    QThreadPool::globalInstance()->start([=]() {

        AppPaths paths = AppPaths::resolve();

#ifdef Q_OS_ANDROID
        AppPaths::extractAssetsIfNeeded(paths.dataDir);
#endif

        loadConfiguration(cfg->m_config, paths.configPath);
        cfg->m_configPath = paths.configPath;

        cfg->m_config.dictPath      = paths.dictPath;
        cfg->m_config.dictIndexPath = paths.dictIndexPath;
        cfg->m_config.glossEnPath   = paths.glossPath("en");
        cfg->m_config.glossEsPath   = paths.glossPath("es");
        cfg->m_config.jpPath        = paths.glossPath("jp");
        cfg->m_config.srsBasePath   = paths.srsBasePath;

        QMetaObject::invokeMethod(controller, [=]() {
            controller->setStatus("Opening dictionary...");
        }, Qt::QueuedConnection);

        auto *repo = new DictionaryRepository();

        if (!repo->open(cfg->m_config.dictPath,
                        cfg->m_config.dictIndexPath)) {
            qWarning("Failed to open dictionary.");
            return;
        }

        kotoba_dict *dict = repo->dict();

        QMetaObject::invokeMethod(controller, [=]() {
            controller->setStatus("Initializing search...");
        }, Qt::QueuedConnection);

        searchSvc->initialize(dict, &cfg->m_config);

        QMetaObject::invokeMethod(controller, [=]() {
            controller->setStatus("Loading SRS...");
        }, Qt::QueuedConnection);

        srsSvc->initialize(
            static_cast<uint32_t>(dict->bin_header->entry_count),
            &cfg->m_config
        );

        srsSvc->load(cfg->m_config.srsBasePath);

        QMetaObject::invokeMethod(qApp, [=]() {

            searchVM->initialize(searchSvc, searchModel, dict, &cfg->m_config);
            detailsVM->initialize(dict, &cfg->m_config);
            srsVM->initialize(srsSvc, dict, detailsVM);
            libVM->initialize(srsSvc, dict, searchSvc, &cfg->m_config);

            cfg->setServices(searchSvc, srsSvc);

            controller->notifyReady();

        }, Qt::QueuedConnection);
    });

    // ── Guardado al cerrar ─────────────────────────────────────
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        srsSvc->save();
        saveConfiguration(configWrapper.m_config, configWrapper.m_configPath);
    });

    int result = app.exec();

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