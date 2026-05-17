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

enum EndingFadeType
{
    ENDING_FADE_TYPE_NO_FADE,
    ENDING_FADE_TYPE_FADE_IN_BLACK,
    ENDING_FADE_TYPE_FADE_OUT_BLACK,
    ENDING_FADE_TYPE_FADE_IN_WHITE,
    ENDING_FADE_TYPE_FADE_OUT_WHITE,
};

#define END_READ_OPCODE '@'
enum EndOpcode
{
    END_OPCODE_FADE_IN_BLACK = '0',
    END_OPCODE_FADE_OUT_BLACK = '1',
    END_OPCODE_FADE_IN = '2',
    END_OPCODE_FADE_OUT = '3',
    END_OPCODE_EXECUTE_ANM = 'a',
    END_OPCODE_BACKGROUND = 'b',
    END_OPCODE_COLOR = 'c',
    END_OPCODE_PLAY_MUSIC = 'm',
    END_OPCODE_WAIT_RESET = 'r',
    END_OPCODE_SET_DELAY = 's',
    END_OPCODE_SET_VERTICAL_SCROLL_POS = 'v',
    END_OPCODE_WAIT = 'w',
    END_OPCODE_END = 'z',
    END_OPCODE_EXEC_END_FILE = 'F',
    END_OPCODE_FADE_MUSIC = 'M',
    END_OPCODE_ROLL_STAFF = 'R',
    END_OPCODE_SCROLL_BACKGROUND = 'V',
};

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
    Float2 backgroundPos;   //0x08 + //0x0c
    f32 backgroundScrollSpeed;  //0x10
    AnmVm vms[16];      //0x14
    char *fileData;     //0x2a54
    ZunBool canSkip;        //0x2a58
    i32 unk2a5c;
    AnmLoaded *anmFile;   //0x2a60
    ZunTimer timer1;     //0x2a64
    ZunTimer timer2;     //0x2a70
    ZunTimer timer3;     //0x2a7c
    i32 minWaitResetFrames;   //0x2a88
    i32 minWaitFrames;    //0x2a8c
    i32 line2Delay;       //0x2a90
    i32 topLineDelay;     //0x2a94

    unknown_fields(0x2a98, 0x4);

    i32 timesFileParsed;    //0x2a9c
    i32 textColor;      //0x2aa0
    D3DCOLOR fadeColor; //0x2aa4
    i32 fadeTimer;      //0x2aa8
    i32 fadeDuration;   //0x2aac
    EndingFadeType fadeMode;       //0x2ab0
    char *cursorPtr;    //0x2ab4
};

C_ASSERT(sizeof(Ending) == 0x2ab8);

} // namespace th08
