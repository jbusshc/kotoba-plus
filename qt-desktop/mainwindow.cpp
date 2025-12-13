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
    db = KP_db_open("./debug/tango.db");
    ui->setupUi(this);
    if (!db) {
        QStandardItem* item = new QStandardItem("No se pudo abrir la base de datos");
        searchResultModel->appendRow(item);
        return;
    }
    searchResultModel = new QStandardItemModel(this);
    ui->searchResultList->setModel(searchResultModel);

    // Ajusta el tamaño de los items
    ui->searchResultList->setUniformItemSizes(true);
    ui->searchResultList->setResizeMode(QListView::Adjust); // ajusta al tamaño del view
    ui->searchResultList->setWordWrap(true);                // si el texto es largo, hace wrap
    ui->searchResultList->setTextElideMode(Qt::ElideNone);  // no corta el texto con "..."
    ui->searchResultList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    ui->searchResultList->setItemDelegate(new ListItemDelegate(this));



    currentEntry = new entry;

    searchTimer = new QTimer(this);
    searchTimer->setSingleShot(true);
    connect(searchTimer, &QTimer::timeout, [this]() {
        QString currentText = ui->lineEditSearch->text();
        searchResultModel->clear();
        if (currentText.trimmed().isEmpty()) {
            return;
        }
        KP_db_text_search(db, currentText.toUtf8().constData(),
                             [](const KPSearchResult* result, void* userdata) {
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
        KP_db_close(db);
    }
    delete currentEntry;
    delete ui;
}

void MainWindow::onSearchTextChanged() {
    searchTimer->start(SEARCH_DEBOUNCE_MS);
}

void MainWindow::handleResult(const KPSearchResult* result) {
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
    KP_db_id_search(db, ent_seq, currentEntry, [](const entry* /*e*/, void* userdata) {
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
    if (!e) return;

    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(ui->detailsLayout);

    // limpiar (sin borrar el último stretch que puedes tener)
    QLayoutItem* child;
    while ((child = layout->takeAt(0))) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    // ============================
    // HEADER — kanji o lectura principal
    // ============================
    QString bigText;

    if (e->k_elements_count > 0) {
        // Hay kanji: mostrar todos los kanjis como título grande y lecturas como subtítulo
        QString allKanjis;
        for (int i = 0; i < e->k_elements_count; i++)
            allKanjis += QString::fromUtf8(e->k_elements[i].keb) + ", ";
        allKanjis.chop(2);

        bigText += QString("<div style='font-size:32px; font-weight:bold;'>%1</div>")
                    .arg(allKanjis);

        if (e->r_elements_count > 0) {
            QString allReadings;
            for (int i = 0; i < e->r_elements_count; i++)
                allReadings += QString::fromUtf8(e->r_elements[i].reb) + ", ";
            allReadings.chop(2);

            bigText += QString("<div style='font-size:20px; color:#888;'>%1</div>")
                        .arg(allReadings);
        }
    } else if (e->r_elements_count > 0) {
        // No hay kanji: mostrar lecturas como título grande, sin subtítulo
        QString allReadings;
        for (int i = 0; i < e->r_elements_count; i++)
            allReadings += QString::fromUtf8(e->r_elements[i].reb) + ", ";
        allReadings.chop(2);

        bigText += QString("<div style='font-size:32px; font-weight:bold;'>%1</div>")
                    .arg(allReadings);
    }

    QLabel* header = new QLabel(bigText);
    header->setTextFormat(Qt::RichText);
    header->setAlignment(Qt::AlignLeft);
    header->setWordWrap(true);
    layout->addWidget(header);

    // ============================
    // SENSES — sin cajas, solo separadores
    // ============================
    for (int i = 0; i < e->senses_count; i++) {
        const sense& s = e->senses[i];

        // Glosas
        if (s.gloss_count > 0) {
            QString glossHTML;
            for (int g = 0; g < s.gloss_count; g++) {
            glossHTML += QString("• %1").arg(QString::fromUtf8(s.gloss[g]));
            if (g < s.gloss_count - 1)
                glossHTML += "<br>";
            }

            QLabel* glossLabel = new QLabel(glossHTML);
            glossLabel->setWordWrap(true);
            glossLabel->setStyleSheet("font-size:16px;");
            layout->addWidget(glossLabel);
        }

        // POS
        if (s.pos_count > 0) {
            QString posText;
            for (int p = 0; p < s.pos_count; p++)
                posText += QString("[%1] ").arg(QString::fromUtf8(s.pos[p]));

            QLabel* posLabel = new QLabel(posText.trimmed());
            posLabel->setStyleSheet("color:#999; font-size:13px;");
            layout->addWidget(posLabel);
        }

        // Separador entre senses (pero no después del último)
        if (i < e->senses_count - 1) {
            QFrame* line = new QFrame();
            line->setFrameShape(QFrame::HLine);
            line->setFrameShadow(QFrame::Sunken);
            layout->addWidget(line);
        }
    }

    layout->addStretch();
    ui->stackedWidget->setCurrentIndex(1);
}


void MainWindow::onBackButtonClicked() {
    ui->stackedWidget->setCurrentIndex(0);
}
