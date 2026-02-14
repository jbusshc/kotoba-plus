#include "search_service.h"

KotobaSearchService::KotobaSearchService(KotobaAppContext *ctx)
    : ctx(ctx), pageSize(20)
{
    init_search_context(&searchContext,
                        ctx->languages,
                        &ctx->dictionary,
                        pageSize);
    warm_up(&searchContext);
}

KotobaSearchService::~KotobaSearchService()
{
    free_search_context(&searchContext);
}

