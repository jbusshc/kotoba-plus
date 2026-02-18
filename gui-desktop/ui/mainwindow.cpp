#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(KotobaAppContext *ctx, QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      context(ctx)
{
    ui->setupUi(this);

    /*
     --------------------------------------------------
     Crear páginas
     --------------------------------------------------
    */

    dictionaryPage = new DictionaryPage(ctx, this);
    detailsPage    = new DetailsPage(this);
    srsDashboard   = new SrsDashboard(this);
    srsPage        = new SrsPage(ctx, this);

    detailsPage->setSrsPresenter(srsPage->getSrsPresenter());

    /*
     --------------------------------------------------
     Añadir al stacked widget
     --------------------------------------------------
    */

    ui->stackedWidget->addWidget(dictionaryPage);   // index 0
    ui->stackedWidget->addWidget(detailsPage);      // index 1
    ui->stackedWidget->addWidget(srsDashboard);     // index 2
    ui->stackedWidget->addWidget(srsPage);          // index 3

    ui->stackedWidget->setCurrentWidget(dictionaryPage);

    setupNavigation();

    /*
     --------------------------------------------------
     FLUJO SRS
     --------------------------------------------------
    */



    // Dashboard -> Start -> Study Page
    connect(srsDashboard, &SrsDashboard::startRequested,
            this, [=]() {
                ui->stackedWidget->setCurrentWidget(srsPage);
                ui->btnDictionary->setChecked(false);
                ui->btnSrs->setChecked(true);
                srsPage->startStudy();
            });

    // Actualizar estadísticas desde SrsPage
    connect(srsPage, &SrsPage::countsUpdated,
            srsDashboard, &SrsDashboard::updateStats);

    // Cuando termina sesión -> volver al dashboard y refrescar
    connect(srsPage, &SrsPage::sessionFinished,
            this, [=]() {
                ui->stackedWidget->setCurrentWidget(srsDashboard);
                srsPage->refreshStats(); // recalcula internamente
            });

    connect(dictionaryPage, &DictionaryPage::entryRequested,
            this, [=](uint32_t entryId) {

        auto details = dictionaryPage->buildEntryDetails(entryId);
        detailsPage->setEntry(details, entryId);

        ui->stackedWidget->setCurrentWidget(detailsPage);
    }); 

    connect(detailsPage, &DetailsPage::backRequested,
            this, [=]() {

                ui->stackedWidget->setCurrentWidget(dictionaryPage);

                ui->btnDictionary->setChecked(true);
                ui->btnSrs->setChecked(false);
            });

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupNavigation()
{
    // Botón Diccionario
    connect(ui->btnDictionary, &QToolButton::clicked,
            this, [=]() {

                ui->btnDictionary->setChecked(true);
                ui->btnSrs->setChecked(false);

                ui->stackedWidget->setCurrentWidget(dictionaryPage);
            });

    // Botón SRS
    connect(ui->btnSrs, &QToolButton::clicked,
            this, [=]() {

                ui->btnDictionary->setChecked(false);
                ui->btnSrs->setChecked(true);

                ui->stackedWidget->setCurrentWidget(srsDashboard);

                // 🔥 recalcular stats reales
                srsPage->refreshStats();
            });

}
