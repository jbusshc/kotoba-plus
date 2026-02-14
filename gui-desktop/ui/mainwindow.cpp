#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QButtonGroup>

MainWindow::MainWindow(KotobaAppContext *ctx, QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Crear páginas
    dictionaryPage = new DictionaryPage(ctx, this);
    detailsPage    = new DetailsPage(this);
    srsPage        = new SrsPage(ctx, this);

    ui->stackedWidget->addWidget(dictionaryPage);
    ui->stackedWidget->addWidget(detailsPage);
    ui->stackedWidget->addWidget(srsPage);

    ui->stackedWidget->setCurrentWidget(dictionaryPage);

    // Navegación tabs
    QButtonGroup *group = new QButtonGroup(this);
    group->setExclusive(true);
    group->addButton(ui->btnDictionary);
    group->addButton(ui->btnSrs);

    ui->btnDictionary->setChecked(true);

    connect(ui->btnDictionary, &QToolButton::clicked, this, [=]() {
        ui->stackedWidget->setCurrentWidget(dictionaryPage);
    });

    // Conexión Dictionary → Details
    connect(dictionaryPage, &DictionaryPage::entryRequested,
            this, [=](uint32_t entryId) {

        auto details = dictionaryPage->buildEntryDetails(entryId);
        detailsPage->setEntry(details);

        ui->stackedWidget->setCurrentWidget(detailsPage);
    });

    // Conexión Back
    connect(detailsPage, &DetailsPage::backRequested,
            this, [=]() {
        ui->stackedWidget->setCurrentWidget(dictionaryPage);
    });


}

MainWindow::~MainWindow()
{
    delete ui;
}
