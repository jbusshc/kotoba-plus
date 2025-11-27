#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <QStandardItem>
#include <QMetaObject>
#include <QToolButton>
#include <QDebug>

static constexpr int SEARCH_DEBOUNCE_MS = 0;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
    db(nullptr), searchResultModel(nullptr), searchTimer(nullptr)
{
    ui->setupUi(this);

    searchResultModel = new QStandardItemModel(this);
    ui->searchResultList->setModel(searchResultModel);

    // Ajusta el tamaño de los items
    ui->searchResultList->setUniformItemSizes(true);
    ui->searchResultList->setResizeMode(QListView::Adjust); // ajusta al tamaño del view
    ui->searchResultList->setWordWrap(true);                // si el texto es largo, hace wrap
    ui->searchResultList->setTextElideMode(Qt::ElideNone);  // no corta el texto con "..."
    ui->searchResultList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    ui->searchResultList->setItemDelegate(new ListItemDelegate(this));

    db = tango_db_open("./debug/tango.db");
    if (!db) {
        QStandardItem* item = new QStandardItem("No se pudo abrir la base de datos");
        searchResultModel->appendRow(item);
        return;
    }
    tango_db_warmup(db);

    currentEntry = new entry;

    searchTimer = new QTimer(this);
    searchTimer->setSingleShot(true);
    connect(searchTimer, &QTimer::timeout, [this]() {
        QString currentText = ui->lineEditSearch->text();
        searchResultModel->clear();
        if (currentText.trimmed().isEmpty()) {
            return;
        }
        tango_db_text_search(db, currentText.toUtf8().constData(),
                             [](const TangoSearchResult* result, void* userdata) {
                                 MainWindow* self = static_cast<MainWindow*>(userdata);
                                 self->handleResult(result);
                             }, this);
    });

    connect(ui->lineEditSearch, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(ui->searchResultList, &QListView::clicked, this, &MainWindow::onSearchResultClicked);
    connect(ui->btnBack, &QToolButton::clicked, this, &MainWindow::onBackButtonClicked);

    // Connect tab buttons for navigation
    connect(ui->toolBtnDiccionario, &QToolButton::clicked, this, [this]() {
        ui->stackedWidget->setCurrentIndex(0); // Dictionary page
        ui->toolBtnDiccionario->setChecked(true);
        ui->toolBtnSRS->setChecked(false);
    });
    connect(ui->toolBtnSRS, &QToolButton::clicked, this, [this]() {
        ui->stackedWidget->setCurrentIndex(2); // SRS page
        ui->toolBtnDiccionario->setChecked(false);
        ui->toolBtnSRS->setChecked(true);
    });

    ui->stackedWidget->setCurrentIndex(0);
    ui->toolBtnDiccionario->setCheckable(true);
    ui->toolBtnDiccionario->setChecked(true);
    ui->toolBtnSRS->setCheckable(true);

}

MainWindow::~MainWindow() {
    if (db) {
        tango_db_close(db);
    }
    delete currentEntry;
    delete ui;
}

void MainWindow::onSearchTextChanged() {
    searchTimer->start(SEARCH_DEBOUNCE_MS);
}

void MainWindow::handleResult(const TangoSearchResult* result) {
    QString kanjis = QString::fromUtf8(result->kanjis).trimmed();
    QString readings = QString::fromUtf8(result->readings).trimmed();
    QString glosses = QString::fromUtf8(result->glosses).trimmed();

    QString topLine = kanjis.isEmpty() ? readings : kanjis + " " + readings;
    QString text = topLine + "\n" + glosses;

    int ent_seq = result->ent_seq;

    QMetaObject::invokeMethod(this, [this, text, ent_seq]() {
        QStandardItem* item = new QStandardItem(text);
        item->setData(ent_seq, Qt::UserRole);
        item->setEditable(false);

        // =============================
        //  PASO 3: Ajustar alto del item
        // =============================
        QFontMetrics fm(ui->searchResultList->font());
        int lines = text.count('\n') + 1;
        int height = fm.lineSpacing() * lines + 12; // padding extra

        // Establece el tamaño del ítem para que ocupe TODO el ancho (luego Qt ajusta)
        item->setSizeHint(QSize(ui->searchResultList->viewport()->width(), height));
        // =============================

        searchResultModel->appendRow(item);
    }, Qt::QueuedConnection);
}


void imprimir_entry(const entry* e, void* userdata) {
    if (!e) return;

    printf("🆔 Entrada %d\n", e->ent_seq);
    printf("   Prioridad: %d\n", e->priority);

    // Kanji elements
    printf("   Kanji(s): ");
    if (e->k_elements_count == 0) {
        printf("(sin kanji)\n");
    } else {
        for (int i = 0; i < e->k_elements_count; i++) {
            printf("%s", e->k_elements[i].keb);
            if (i < e->k_elements_count - 1) printf(", ");
        }
        printf("\n");
    }

    // Reading elements
    printf("   Lectura(s): ");
    if (e->r_elements_count == 0) {
        printf("(sin lectura)\n");
    } else {
        for (int i = 0; i < e->r_elements_count; i++) {
            printf("%s", e->r_elements[i].reb);
            if (i < e->r_elements_count - 1) printf(", ");
        }
        printf("\n");
    }

    // Senses
    for (int i = 0; i < e->senses_count; i++) {
        printf("   Sentido %d:\n", i + 1);
        printf("      Glosas: ");
        for (int j = 0; j < e->senses[i].gloss_count; j++) {
            printf("%s", e->senses[i].gloss[j]);
            if (j < e->senses[i].gloss_count - 1) printf("; ");
        }
        printf("\n");
        if (e->senses[i].pos_count > 0) {
            printf("      POS: ");
            for (int j = 0; j < e->senses[i].pos_count; j++) {
                printf("%s", e->senses[i].pos[j]);
                if (j < e->senses[i].pos_count - 1) printf(", ");
            }
            printf("\n");
        }
        if (e->senses[i].misc_count > 0) {
            printf("      Misc: ");
            for (int j = 0; j < e->senses[i].misc_count; j++) {
                printf("%s", e->senses[i].misc[j]);
                if (j < e->senses[i].misc_count - 1) printf(", ");
            }
            printf("\n");
        }
    }
    printf("\n");
}

void MainWindow::onSearchResultClicked(const QModelIndex &index) {
    if (!index.isValid()) return;

    int ent_seq = index.data(Qt::UserRole).toInt();
    qDebug() << "Clicked ent_seq:" << ent_seq;
    memset(currentEntry, 0, sizeof(entry));
    tango_db_id_search(db, ent_seq, currentEntry, [](const entry* /*e*/, void* userdata) {
        MainWindow* self = static_cast<MainWindow*>(userdata);

        // Esto ocurre en el siguiente ciclo de eventos
        QMetaObject::invokeMethod(self, [self]() {
            self->showEntryDetails(self->currentEntry);

            // Mover de página aquí, no antes
            self->ui->stackedWidget->setCurrentIndex(1);
            self->ui->searchResultList->clearSelection();
        }, Qt::QueuedConnection);
    }, this);
}


void MainWindow::showEntryDetails(const entry* e) {
    qDebug() << "Mostrando detalles para entry:" << e->ent_seq;
    if (!e) return;

    QString detailText;
    detailText += QString("Entry ID: %1\n\n").arg(e->ent_seq);

    detailText += "Kanjis:\n";
    for (int i = 0; i < e->k_elements_count; i++) {
        detailText += QString(" - %1\n").arg(QString::fromUtf8(e->k_elements[i].keb));
    }

    detailText += "\nReadings:\n";
    for (int i = 0; i < e->r_elements_count; i++) {
        detailText += QString(" - %1\n").arg(QString::fromUtf8(e->r_elements[i].reb));
    }

    detailText += "\nSenses:\n";
    for (int i = 0; i < e->senses_count; i++) {
        const sense &s = e->senses[i];
        for (int j = 0; j < s.gloss_count; j++) {
            detailText += QString(" - %1\n").arg(QString::fromUtf8(s.gloss[j]));
        }
        detailText += "\n";
    }

    ui->detailTextEdit->setText(detailText);
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::onBackButtonClicked() {
    ui->stackedWidget->setCurrentIndex(0);
}
