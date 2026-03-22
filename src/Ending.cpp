#include "Ending.hpp"

namespace th08
{

ZunResult Ending::ReadEndFileParameter()
{
    return ZUN_ERROR;
}

void Ending::FadingEffect()
{
}

ZunResult Ending::ParseEndFile()
{
    return ZUN_ERROR;
}

ZunResult Ending::LoadEnding(const char *path)
{
    return ZUN_ERROR;
}

ZunResult Ending::RegisterChain()
{
    return ZUN_ERROR;
}

ChainCallbackResult Ending::OnUpdate(Ending *ending)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult Ending::OnDraw(Ending *ending)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ZunResult Ending::AddedCallback(Ending *ending)
{
    return ZUN_ERROR;
}

ZunResult Ending::DeletedCallback(Ending *ending)
{
    return ZUN_ERROR;
}

} /* namespace th08 */