#ifndef DETAILSPAGE_H
#define DETAILSPAGE_H

#include <QWidget>
#include "../../search/search_presenter.h"  // Para EntryDetails
#include "../../srs/srs_presenter.h"  // Para Srs add/remove/contains

namespace Ui {
class DetailsPage;
}

class DetailsPage : public QWidget
{
    Q_OBJECT

public:
    explicit DetailsPage(QWidget *parent = nullptr);
    ~DetailsPage();

    void setEntry(const EntryDetails &details, uint32_t entryId);
    void setSrsPresenter(SrsPresenter *presenter);

signals:
    void backRequested();

private slots:
    void onSrsButtonClicked();


private:
    void updateSrsButton();

    Ui::DetailsPage *ui;
    SrsPresenter *srsPresenter = nullptr;
    uint32_t currentEntryId = 0; // almacenar el ID del entry mostrado para acciones SRS
};

#endif
