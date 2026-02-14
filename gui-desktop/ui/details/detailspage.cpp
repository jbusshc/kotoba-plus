#include "detailspage.h"
#include "ui_detailspage.h"

DetailsPage::DetailsPage(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::DetailsPage)
{
    ui->setupUi(this);

    connect(ui->btnBack, &QToolButton::clicked,
            this, &DetailsPage::backRequested);
}

DetailsPage::~DetailsPage()
{
    delete ui;
}

void DetailsPage::setEntry(const EntryDetails &details)
{
    ui->labelMainWord->setText(details.mainWord);
    ui->labelReadings->setText(details.readings);

    ui->listSenses->clear();
    for (const auto &sense : details.senses)
        ui->listSenses->addItem(sense);
}
