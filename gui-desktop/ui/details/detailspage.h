#ifndef DETAILSPAGE_H
#define DETAILSPAGE_H

#include <QWidget>
#include "../../search/search_presenter.h"  // Para EntryDetails

namespace Ui {
class DetailsPage;
}

class DetailsPage : public QWidget
{
    Q_OBJECT

public:
    explicit DetailsPage(QWidget *parent = nullptr);
    ~DetailsPage();

    void setEntry(const EntryDetails &details);

signals:
    void backRequested();

private:
    Ui::DetailsPage *ui;
};

#endif
