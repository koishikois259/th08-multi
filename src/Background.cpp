#include "Background.hpp"

namespace th08
{
DIFFABLE_STATIC(Background, g_Background);

Background::Background()
{
}

ChainCallbackResult Background::OnUpdate(Background *background)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult Background::OnDrawHighPrio(Background *background)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult Background::OnDrawLowPrio(Background *background)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ZunResult Background::AddedCallback(Background *background)
{
    return ZUN_ERROR;
}

ZunResult Background::RegisterChain()
{
    return ZUN_ERROR;
}

ZunResult Background::DeletedCallback()
{
    return ZUN_ERROR;
}

void Background::CutChain()
{
}

ZunResult Background::LoadStageData()
{
    return ZUN_ERROR;
}

}; // Namespace th08
