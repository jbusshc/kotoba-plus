#include "srsdashboard.h"
#include "ui_srsdashboard.h"

SrsDashboard::SrsDashboard(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::SrsDashboard)
{
    ui->setupUi(this);

    connect(ui->btnStart, &QPushButton::clicked,
            this, &SrsDashboard::startRequested);
}

SrsDashboard::~SrsDashboard()
{
    delete ui;
}

void SrsDashboard::updateStats(uint32_t due,
                               uint32_t learning,
                               uint32_t newly,
                               uint32_t lapsed)
{
    ui->labelDue->setText(QString("Due: %1").arg(due));
    ui->labelLearning->setText(QString("Learning: %1").arg(learning));
    ui->labelNew->setText(QString("New: %1").arg(newly));
    ui->labelLapsed->setText(QString("Lapsed: %1").arg(lapsed));

    // 🔥 lógica correcta
    ui->btnStart->setEnabled(due > 0);
}
