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

#include "Configuration.h"

int main(int argc, char **argv)
{
    ConfigWrapper configWrapper;

    // Ruta de config en la misma carpeta del ejecutable
    QString exeDir = QCoreApplication::applicationDirPath();
    QString configPath = QDir(exeDir).filePath("config.ini");

    qDebug() << "Config file path:" << configPath;
    qDebug() << "File exists:" << QFile::exists(configPath);

    bool b = loadConfiguration(configWrapper.m_config, configPath);

    QQuickStyle::setStyle("Material");
    QGuiApplication app(argc, argv);
    app.setApplicationName("KotobaPlus");

    qDebug() << "Working dir:" << QDir::currentPath();
    qDebug() << "App dir:" << QCoreApplication::applicationDirPath();

    // create app-data folder
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appData.isEmpty()) appData = QDir::currentPath();
    QDir().mkpath(appData);

    QString base = QCoreApplication::applicationDirPath();

    // Determine dict path: prefer appData, fallback to cwd
    QString dictFile = base + "/dict.kotoba";
    QString dictIdxFile = base + "/dict.kotoba.idx";
    if (!QFile::exists(dictFile))
        dictFile = QDir::current().filePath("dict.kotoba");
    if (!QFile::exists(dictIdxFile))
        dictIdxFile = QDir::current().filePath("dict.kotoba.idx");

    // Repository (wraps loader)
    DictionaryRepository *repo = new DictionaryRepository();
    if (!repo->open(dictFile, dictIdxFile)) {
        qWarning("Failed to open dictionary files. Ensure dict.kotoba and dict.kotoba.idx are present.");
        // proceed anyway: SearchService will be invalid (guarded)
    }

    kotoba_dict *dict = repo->dict();

    // Services
    SearchService *searchSvc = new SearchService(dict, &configWrapper.m_config);
    SrsService *srsSvc = new SrsService(dict->entry_count, &configWrapper.m_config);

    // Models & ViewModels
    SearchResultModel *searchModel = new SearchResultModel();
    SearchViewModel *searchVM = new SearchViewModel(searchSvc, searchModel, dict);
    EntryDetailsViewModel *detailsVM = new EntryDetailsViewModel(dict, &configWrapper.m_config);
    SrsViewModel *srsVM = new SrsViewModel(srsSvc, dict);

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("searchVM", searchVM);
    engine.rootContext()->setContextProperty("searchModel", searchModel);
    engine.rootContext()->setContextProperty("detailsVM", detailsVM);
    engine.rootContext()->setContextProperty("srsVM", srsVM);
    engine.rootContext()->setContextProperty("appDataPath", appData);
    engine.rootContext()->setContextProperty("appConfig", &configWrapper);

    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) return -1;

    const char* srsProfilePath = configWrapper.m_config.srsPath.toStdString().c_str();
    printf("Loading SRS profile...\n");
    srsSvc->load(srsProfilePath);

     // Ensure profile is saved on exit (persisting any sync state)
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        printf("Saving SRS profile...\n");
        srsSvc->save(srsProfilePath);
        saveConfiguration(configWrapper.m_config, configPath.toStdString().c_str());
    });
    
    int result = app.exec();

    delete searchVM;
    delete searchModel;
    delete detailsVM;
    delete srsVM;
    delete searchSvc;
    delete srsSvc;
    delete repo;

    return result;
}