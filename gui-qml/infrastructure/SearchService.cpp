#include "SearchService.h"
#include <QDebug>
#include <QMetaObject>
#include <QThreadPool>
#include <cstring>
#include "../app/Configuration.h"

// ── Constructor / Destructor ────────────────────────────────────────────────
SearchService::SearchService(QObject *parent)
    : QObject(parent), m_dict(nullptr), m_config(nullptr)
{
}

SearchService::~SearchService()
{
    free_search_context(&m_ctx);
}

// ── Inicialización ──────────────────────────────────────────────────────────
void SearchService::initialize(kotoba_dict *dict, Configuration* config)
{
    m_dict = dict;
    m_config = config;
    init_search_context(&m_ctx, config->languages, m_dict, config->pageSize, config->maxResults);

    qDebug() << "SearchService initialized with languages:";
    for (int i = 0; i < KOTOBA_LANG_COUNT; ++i) {
        if (config->languages[i] != 0)
            qDebug() << i << "-" << config->languages[i];
    }

    warm_up(&m_ctx);
}

// ── Configuración ──────────────────────────────────────────────────────────
void SearchService::updateConfig(const Configuration* config)
{
    m_config = config;
    update_search_config(&m_ctx, config->languages, config->pageSize, config->maxResults);
}

// ── Funciones de búsqueda ────────────────────────────────────────────────────
void SearchService::queryAsync(const QString &q)
{
    QThreadPool::globalInstance()->start([this, q]() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (q.isEmpty()) {
            reset_search_context(&m_ctx);
        } else {
            QByteArray utf8 = q.toUtf8();
            if (utf8.size() >= MAX_QUERY_LEN)
                utf8.truncate(MAX_QUERY_LEN - 1);
            query_results(&m_ctx, utf8.constData());
            query_next_page(&m_ctx);
        }

        QMetaObject::invokeMethod(this, [this]() {
            emit searchDone();
        }, Qt::QueuedConnection);
    });
}

void SearchService::query(const QString &q)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (q.isEmpty()) {
        reset_search_context(&m_ctx);
        return;
    }

    QByteArray utf8 = q.toUtf8();
    if (utf8.size() >= MAX_QUERY_LEN)
        utf8.truncate(MAX_QUERY_LEN - 1);

    query_results(&m_ctx, utf8.constData());
    query_next_page(&m_ctx);
}

void SearchService::queryNonPagination(const QString &q)
{
    QThreadPool::globalInstance()->start([this, q]() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (q.isEmpty()) {
            reset_search_context(&m_ctx);
        } else {
            QByteArray utf8 = q.toUtf8();
            if (utf8.size() >= MAX_QUERY_LEN)
                utf8.truncate(MAX_QUERY_LEN - 1);
            query_results(&m_ctx, utf8.constData());
        }

        QMetaObject::invokeMethod(this, [this]() {
            emit searchDone();
        }, Qt::QueuedConnection);
    });
}

void SearchService::queryNextPage()
{
    QThreadPool::globalInstance()->start([this]() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_ctx.results_left)
            query_next_page(&m_ctx);

        QMetaObject::invokeMethod(this, [this]() {
            emit pageReady();
        }, Qt::QueuedConnection);
    });
}

// ── Últimas variantes de búsqueda ───────────────────────────────────────────
QueryVariants SearchService::lastVariants() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    QueryVariants v;
    v.normal   = QString::fromUtf8(m_ctx.queries_buffer,
                                   strnlen(m_ctx.queries_buffer, MAX_QUERY_LEN));
    v.romaji   = QString::fromUtf8(m_ctx.mixed_query,
                                   strnlen(m_ctx.mixed_query,   MAX_QUERY_LEN));
    v.hiragana = QString::fromUtf8(m_ctx.variant_query,
                                   strnlen(m_ctx.variant_query, MAX_QUERY_LEN));
    return v;
}

// ── Colores de acento ───────────────────────────────────────────────────────
static QString accentToHex(const QString &name)
{
    static const QHash<QString, QString> table = {
        { "red",         "#F44336" }, { "pink",        "#E91E63" },
        { "purple",      "#9C27B0" }, { "deeppurple",  "#673AB7" },
        { "indigo",      "#3F51B5" }, { "blue",        "#2196F3" },
        { "lightblue",   "#03A9F4" }, { "cyan",        "#00BCD4" },
        { "teal",        "#009688" }, { "green",       "#4CAF50" },
        { "lightgreen",  "#8BC34A" }, { "lime",        "#CDDC39" },
        { "yellow",      "#FFEB3B" }, { "amber",       "#FFC107" },
        { "orange",      "#FF9800" }, { "deeporange",  "#FF5722" },
        { "brown",       "#795548" }, { "bluegrey",    "#607D8B" },
    };
    return table.value(name, "#2196F3");
}

// ── Comparación de kana ─────────────────────────────────────────────────────
static bool kanaEqualQString(const QString &a, const QString &b)
{
    QByteArray aUtf8 = a.toUtf8();
    QByteArray bUtf8 = b.toUtf8();
    return kana_equal(aUtf8.constData(), bUtf8.constData());
}

static int kanaIndexOf(const QString &text, const QString &pattern)
{
    if (pattern.isEmpty()) return -1;

    for (int i = 0; i <= text.size() - pattern.size(); ++i)
    {
        QStringView slice(text.constData() + i, pattern.size());
        if (kanaEqualQString(slice.toString(), pattern))
            return i;
    }

    return -1;
}

// ── Resaltado de coincidencias ──────────────────────────────────────────────
QString SearchService::highlightField(const QString &field) const
{
    const auto variants = lastVariants();
    const QString fieldLower = field.toLower();
    int matchIdx = -1;
    int matchLen = 0;

    for (const QString &v : { variants.normal, variants.romaji, variants.hiragana }) {
        if (v.isEmpty()) continue;
        const QString vLower = v.toLower();
        int idx = kanaIndexOf(fieldLower, vLower);
        if (idx >= 0 && (matchIdx < 0 || idx < matchIdx)) {
            matchIdx = idx;
            matchLen = v.length();
        }
    }

    if (matchIdx < 0) return field;

    const QString colorHex = m_config ? accentToHex(m_config->accentColor) : QStringLiteral("#2196F3");

    QString result;
    result += field.left(matchIdx).toHtmlEscaped();
    result += QStringLiteral("<b style=\"color:") + colorHex + QStringLiteral("\">");
    result += field.mid(matchIdx, matchLen).toHtmlEscaped();
    result += QStringLiteral("</b>");
    result += field.mid(matchIdx + matchLen).toHtmlEscaped();
    return result;
}