#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QStandardItem>
#include <QToolButton>

static constexpr int SEARCH_DEBOUNCE_MS = 200;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    searchResultModel(new QStandardItemModel(this)),
    searchTimer(new QTimer(this))
{
    ui->setupUi(this); // 🔴 ESTO FALTABA

    // Modelo de resultados
    ui->searchResultList->setModel(searchResultModel);
    ui->searchResultList->setWordWrap(true);
    ui->searchResultList->setUniformItemSizes(false);

    // Timer debounce
    searchTimer->setSingleShot(true);
    connect(searchTimer, &QTimer::timeout, this, [this]() {
        searchResultModel->clear();

        QString text = ui->lineEditSearch->text().trimmed();
        if (text.isEmpty())
            return;

        // RESULTADOS DUMMY (para verificar UI)
        for (int i = 0; i < 5; ++i) {
            QStandardItem *item = new QStandardItem(
                QString("Resultado %1\nSignificado de \"%2\"")
                    .arg(i + 1)
                    .arg(text)
                );
            item->setEditable(false);
            searchResultModel->appendRow(item);
        }
    });

    // Conexiones UI
    connect(ui->lineEditSearch, &QLineEdit::textChanged,
            this, &MainWindow::onSearchTextChanged);

    connect(ui->searchResultList, &QListView::clicked,
            this, &MainWindow::onSearchResultClicked);

    connect(ui->btnBack, &QToolButton::clicked,
            this, &MainWindow::onBackButtonClicked);

    // Navegación tabs
    connect(ui->toolBtnDiccionario, &QToolButton::clicked, this, [this]() {
        ui->stackedWidget->setCurrentIndex(0);
    });

    connect(ui->toolBtnSRS, &QToolButton::clicked, this, [this]() {
        ui->stackedWidget->setCurrentIndex(2);
    });

    ui->stackedWidget->setCurrentIndex(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onSearchTextChanged()
{
    searchTimer->start(SEARCH_DEBOUNCE_MS);
}

void MainWindow::onSearchResultClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    // Por ahora solo navega
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::onBackButtonClicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}
