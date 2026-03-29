#pragma once

#include "Global.hpp"
#include "ZunResult.hpp"
#include "diffbuild.hpp"
#include "inttypes.hpp"
#include "utils.hpp"

namespace th08
{

struct Ending
{
    ZunResult ReadEndFileParameter();
    void FadingEffect();
    ZunResult ParseEndFile();
    ZunResult LoadEnding(const char *path);

    static ZunResult RegisterChain();
    static ChainCallbackResult OnUpdate(Ending *ending);
    static ChainCallbackResult OnDraw(Ending *ending);
    static ZunResult AddedCallback(Ending *ending);
    static ZunResult DeletedCallback(Ending *ending);

    unknown_fields(0x0, 0x2ab8);
};

C_ASSERT(sizeof(Ending) == 0x2ab8);

} // namespace th08
