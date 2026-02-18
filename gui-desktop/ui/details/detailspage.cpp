#include "detailspage.h"
#include "ui_detailspage.h"

DetailsPage::DetailsPage(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::DetailsPage)
{
    ui->setupUi(this);

    connect(ui->btnBack, &QToolButton::clicked,
            this, &DetailsPage::backRequested);

    connect(ui->btnSrsAction, &QPushButton::clicked,
            this, &DetailsPage::onSrsButtonClicked);
}

DetailsPage::~DetailsPage()
{
    delete ui;
}

void DetailsPage::setSrsPresenter(SrsPresenter *presenter)
{
    srsPresenter = presenter;
}

void DetailsPage::setEntry(const EntryDetails &details, uint32_t entryId)
{
    currentEntryId = entryId;

    ui->labelMainWord->setText(details.mainWord);
    ui->labelReadings->setText(details.readings);

    ui->listSenses->clear();
    for (const auto &sense : details.senses)
        ui->listSenses->addItem(sense);

    updateSrsButton();
}

void DetailsPage::updateSrsButton()
{
    if (!srsPresenter)
    {
        ui->btnSrsAction->setVisible(false);
        return;
    }

    ui->btnSrsAction->setVisible(true);

    bool inSrs = srsPresenter->contains(currentEntryId);

    if (inSrs)
        ui->btnSrsAction->setText("Olvidar carta");
    else
        ui->btnSrsAction->setText("Añadir al SRS");
}

void DetailsPage::onSrsButtonClicked()
{
    if (!srsPresenter)
        return;

    bool inSrs = srsPresenter->contains(currentEntryId);

    if (inSrs)
        srsPresenter->remove(currentEntryId);
    else
        srsPresenter->add(currentEntryId);

    updateSrsButton();
}
