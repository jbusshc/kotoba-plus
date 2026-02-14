#ifndef DICTIONARYPAGE_H
#define DICTIONARYPAGE_H

#include <QWidget>
#include <QTimer>
#include <QModelIndex>

#include "../../app/context.h"
#include "../../search/presenter.h"
#include "../../search/service.h"
#include "../../search/result_model.h"

namespace Ui {
class DictionaryPage;
}

class DictionaryPage : public QWidget
{
    Q_OBJECT

public:
    explicit DictionaryPage(KotobaAppContext *ctx, QWidget *parent = nullptr);
    ~DictionaryPage();

    EntryDetails buildEntryDetails(uint32_t entryId);

signals:
    void entryRequested(uint32_t entryId);

private slots:
    void onSearchTextChanged();
    void onSearchResultClicked(const QModelIndex &index);
    void onEntrySelected(uint32_t entryId);

private:
    Ui::DictionaryPage *ui;

    QTimer *searchTimer;

    SearchPresenter *presenter;
    SearchResultModel *searchResultModel;
};

#endif
