#pragma once
#include "diffbuild.hpp"
#include "inttypes.hpp"
#include "utils.hpp"
#include "ZunResult.hpp"
#include "Global.hpp"
#include <windows.h>

namespace th08
{
struct Background
{
    Background();

    static ChainCallbackResult OnUpdate(Background *background);
    static ChainCallbackResult OnDrawHighPrio(Background *background);
    static ChainCallbackResult OnDrawLowPrio(Background *background);
    static ZunResult AddedCallback(Background *background);
    static ZunResult RegisterChain();
    static ZunResult DeletedCallback();
    static void CutChain();
    ZunResult LoadStageData();

    void SetCamera1()
    {
    }

    void SetCamera2()
    {
    }

    unknown_fields(0x0, 0xb20);
    u8 skyFogNeedsSetup; // Leftover from earlier games. Never checked in IN
    unknown_fields(0xb21, 0x5adf);
};
C_ASSERT(sizeof(Background) == 0x6600);

DIFFABLE_EXTERN(Background, g_Background);
}; // Namespace th08
