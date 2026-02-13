#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QToolButton>
#include <QStandardItem>

static constexpr int SEARCH_DEBOUNCE_MS = 0;

MainWindow::MainWindow(KotobaAppContext *ctx, QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      searchTimer(new QTimer(this))
{
    ui->setupUi(this);

    // ===== MVP =====
    auto *searchService = new KotobaSearchService(ctx);
    presenter = new SearchPresenter(searchService, this);

    searchResultModel = new SearchResultModel(presenter, this);

    ui->searchResultList->setModel(searchResultModel);
    ui->searchResultList->setWordWrap(true);
    ui->searchResultList->setUniformItemSizes(false);

    // ===== Debounce =====
    searchTimer->setSingleShot(true);
    connect(searchTimer, &QTimer::timeout, this, [this]() {
        presenter->onSearchTextChanged(
            ui->lineEditSearch->text().trimmed());
    });

    // ===== UI signals =====
    connect(ui->lineEditSearch, &QLineEdit::textChanged,
            this, &MainWindow::onSearchTextChanged);

    connect(ui->searchResultList, &QListView::clicked,
            this, &MainWindow::onSearchResultClicked);

    connect(ui->btnBack, &QToolButton::clicked,
            this, &MainWindow::onBackButtonClicked);

    connect(presenter, &SearchPresenter::entrySelected,
            this, &MainWindow::showEntry);


    ui->stackedWidget->setCurrentIndex(0);
}


MainWindow::~MainWindow()
{
    delete presenter;
    delete ui;
}


void MainWindow::showEntry(uint32_t entryId)
{
    auto details = presenter->buildEntryDetails(entryId);

    ui->labelMainWord->setText(details.mainWord);
    ui->labelReadings->setText(details.readings);

    ui->listSenses->clear();
    for (const auto &sense : details.senses)
        ui->listSenses->addItem(sense);

    ui->stackedWidget->setCurrentWidget(ui->pageDetails);
}

void MainWindow::onSearchTextChanged()
{
    searchTimer->start(SEARCH_DEBOUNCE_MS);
}

void MainWindow::onSearchResultClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    presenter->onResultClicked(index.row());
}

void MainWindow::onBackButtonClicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}
