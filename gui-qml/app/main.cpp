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

#include <stdlib.h>

int main(int argc, char **argv)
{
    srand(static_cast<unsigned int>(time(nullptr)));

    // ── QGuiApplication must be created before QStandardPaths on Android ─────
    QGuiApplication app(argc, argv);
    app.setApplicationName("KotobaPlus");
    app.setOrganizationName("KotobaPlus");   // required for correct AppDataLocation on Android

    QQuickStyle::setStyle("Material");

    // ── Resolve all platform-specific paths ───────────────────────────────────
    AppPaths paths = AppPaths::resolve();

    qDebug() << "Config path:"   << paths.configPath;
    qDebug() << "Data dir:"      << paths.dataDir;
    qDebug() << "SRS profile:"   << paths.srsPath;

    // ── On Android: extract bundled assets to writable storage on first run ───
    // Assets in Qt Android are inside the APK and cannot be opened with
    // fopen(). They must be copied to a writable location first.
#ifdef Q_OS_ANDROID
    AppPaths::extractAssetsIfNeeded(paths.dataDir);
#endif

    // ── Configuration ─────────────────────────────────────────────────────────
    ConfigWrapper configWrapper;
    loadConfiguration(configWrapper.m_config, paths.configPath);
    configWrapper.m_configPath = paths.configPath;

    // Override data paths from resolved AppPaths so the config struct always
    // points to writable, correct locations regardless of platform.
    configWrapper.m_config.dictPath      = paths.dictPath;
    configWrapper.m_config.dictIndexPath = paths.dictIndexPath;
    configWrapper.m_config.srsPath       = paths.srsPath;
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

    // ── Dictionary ────────────────────────────────────────────────────────────
    DictionaryRepository *repo = new DictionaryRepository();
    if (!repo->open(configWrapper.m_config.dictPath,
                    configWrapper.m_config.dictIndexPath)) {
        qWarning("Failed to open dictionary. Ensure data files are present.");
        return -1;
    }
    kotoba_dict *dict = repo->dict();

    // ── Services ──────────────────────────────────────────────────────────────
    SearchService *searchSvc = new SearchService(dict, &configWrapper.m_config);
    SrsService    *srsSvc    = new SrsService(dict->entry_count, &configWrapper.m_config);

    // ── ViewModels ────────────────────────────────────────────────────────────
    SearchResultModel     *searchModel = new SearchResultModel();
    SearchViewModel       *searchVM   = new SearchViewModel(searchSvc, searchModel, dict, &configWrapper.m_config);
    EntryDetailsViewModel *detailsVM  = new EntryDetailsViewModel(dict, &configWrapper.m_config);
    SrsViewModel          *srsVM      = new SrsViewModel(srsSvc, dict, detailsVM);
    SrsLibraryViewModel   *libVM      = new SrsLibraryViewModel(srsSvc, dict, searchSvc, &configWrapper.m_config);
    // ── QML engine ────────────────────────────────────────────────────────────
    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("searchVM",    searchVM);
    engine.rootContext()->setContextProperty("searchModel", searchModel);
    engine.rootContext()->setContextProperty("detailsVM",   detailsVM);
    engine.rootContext()->setContextProperty("srsVM",       srsVM);
    engine.rootContext()->setContextProperty("appConfig",   &configWrapper);
    engine.rootContext()->setContextProperty("srsLibraryVM", libVM);

    // QML is always loaded from the Qt resource system (qrc:/) — identical
    // on desktop and Android. No platform-specific URL needed.
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) return -1;

    // ── Load SRS profile ──────────────────────────────────────────────────────
    configWrapper.setServices(searchSvc, srsSvc);
    std::string srsProfilePath = configWrapper.m_config.srsPath.toStdString();
    srsSvc->load(srsProfilePath.c_str());

    // ── Persist on exit ───────────────────────────────────────────────────────
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        srsSvc->save(srsProfilePath.c_str());
        saveConfiguration(configWrapper.m_config, paths.configPath);
    });

    int result = app.exec();

    delete searchVM;
    delete searchModel;
    delete detailsVM;
    delete srsVM;
    delete libVM;
    delete searchSvc;
    delete srsSvc;
    delete repo;

    return result;
}
