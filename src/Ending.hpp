#pragma once

#include "Global.hpp"
#include "ZunResult.hpp"
#include "diffbuild.hpp"
#include "inttypes.hpp"
#include "utils.hpp"
#include "ScreenEffect.hpp"
#include "Supervisor.hpp"
#include "AnmManager.hpp"
#include <d3dx8.h>

namespace th08
{

struct Ending
{
    Ending();

    ZunResult ReadEndFileParameter();
    void FadingEffect();
    ZunResult ParseEndFile();
    ZunResult LoadEnding(const char *path);

    static ZunResult RegisterChain();
    static ChainCallbackResult OnUpdate(Ending *ending);
    static ChainCallbackResult OnDraw(Ending *ending);
    static ZunResult AddedCallback(Ending *ending);
    static ZunResult DeletedCallback(Ending *ending);
    
    ChainElem *calcChain; //0x0
    ChainElem *drawChain; //0x4
    i32 unk08;
    i32 unk0c;
    i32 unk10;
    AnmVm vms[16];      //0x14
    void *fileData;     //0x2a54
    i32 canSkip;        //0x2a58

    unknown_fields(0x2a5c, 0x8);

    ZunTimer timer1;     //0x2a64
    ZunTimer timer2;     //0x2a70
    ZunTimer timer3;     //0x2a7c

    unknown_fields(0x2a88, 0x8);

    i32 unknown_2a90;    //0x2a90

    unknown_fields(0x2a94, 0x10);

    D3DCOLOR fadeColor; //0x2aa4
    i32 fadeTimer;      //0x2aa8
    i32 fadeDuration;   //0x2aac
    i32 fadeMode;       //0x2ab0
    char *cursorPtr;    //0x2ab4
};

C_ASSERT(sizeof(Ending) == 0x2ab8);

} // namespace th08
