#include "dictionarypage.h"
#include "ui_dictionarypage.h"

static constexpr int SEARCH_DEBOUNCE_MS = 0;

DictionaryPage::DictionaryPage(KotobaAppContext *ctx, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::DictionaryPage),
      searchTimer(new QTimer(this))
{
    ui->setupUi(this);

    // ===== MVP =====
    auto *searchService = new KotobaSearchService(ctx);
    presenter = new SearchPresenter(searchService, this);
    searchResultModel = new SearchResultModel(presenter, this);

    ui->listResults->setModel(searchResultModel);
    ui->listResults->setWordWrap(true);
    ui->listResults->setUniformItemSizes(false);

    // ===== Debounce =====
    searchTimer->setSingleShot(true);

    connect(searchTimer, &QTimer::timeout, this, [this]() {
        presenter->onSearchTextChanged(
            ui->lineEditSearch->text().trimmed());
    });

    connect(ui->lineEditSearch, &QLineEdit::textChanged,
            this, &DictionaryPage::onSearchTextChanged);

    connect(ui->listResults, &QListView::clicked,
            this, &DictionaryPage::onSearchResultClicked);

    connect(presenter, &SearchPresenter::entrySelected,
            this, &DictionaryPage::onEntrySelected);
}

DictionaryPage::~DictionaryPage()
{
    delete presenter;
    delete ui;
}

void DictionaryPage::onSearchTextChanged()
{
    searchTimer->start(SEARCH_DEBOUNCE_MS);
}

void DictionaryPage::onSearchResultClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    presenter->onResultClicked(index.row());
}

void DictionaryPage::onEntrySelected(uint32_t entryId)
{
    emit entryRequested(entryId);
}

EntryDetails DictionaryPage::buildEntryDetails(uint32_t entryId)
{
    return presenter->buildEntryDetails(entryId);
}
