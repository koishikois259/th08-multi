#pragma once

#include <windows.h>

#include "ZunResult.hpp"
#include "diffbuild.hpp"
#include "inttypes.hpp"
#include "zwave.hpp"

namespace th08
{
enum SoundIdx
{
};

struct SoundPlayerCommand
{
    i32 arg1;
    i32 arg2;
    i32 arg3;
    char string[256];
};

class SoundPlayer
{
  public:
    SoundPlayer();

    ZunResult InitializeDSound(HWND window);
    ZunResult InitSoundBuffers();
    ZunResult Release();

    ZunResult LoadSound(i32 idx, char *path);
    static WAVEFORMATEX *GetWavFormatData(u8 *soundData, char *formatString, i32 *formatSize,
                                          u32 fileSizeExcludingFormat);

    void QueueCommand(i32 opcode, i32 arg1, i32 arg2, char *unused);
    i32 ProcessQueues();
    void PlaySoundByIdx(SoundIdx idx, i32 unused);
    void PlaySoundPositionedByIdx(SoundIdx idx, f32 pan);
    ZunResult StartBGM(char *path);
    ZunResult ReopenBGM(char *path);
    ZunResult PreloadBGM(char *path);
    ZunResult LoadBGM(char *path);
    void FreePreloadedBGM();
    void StopBGM();
    void FadeOut(f32 seconds);

    static DWORD WINAPI BGMPlayerThread(LPVOID lpThreadParameter);

    i32 GetFmtIndexByName(char *name);
    ZunResult LoadFmt(char *path);

    LPDIRECTSOUND dsoundHdl;
    i32 unk4;
    LPDIRECTSOUNDBUFFER soundBuffers[128];
    LPDIRECTSOUNDBUFFER duplicateSoundBuffers[128];
    i32 unk408[128];
    LPDIRECTSOUNDBUFFER initSoundBuffer;
    HWND gameWindow;
    CSoundManager *manager;
    DWORD bgmThreadId;
    HANDLE bgmThreadHandle;
    i32 unk61c;
    i32 soundBuffersToPlay[12];
    u32 unk650[12];
    u32 unk680[12][128];
    ThBgmFormat *unk1e80[16];
    LPBYTE unk1ec0[16];
    LPBYTE unk1f00[16];
    DWORD bgmPreloadAllocSizes[16];
    ThBgmFormat *bgmFmtData;
    SoundPlayerCommand commandQueue[32];
    char bgmFileNames[16][256];
    char currentBgmFileName[256];
    CStreamingSound *bgm;
    HANDLE bgmUpdateEvent;
    i32 unk5210;
    u32 unusedBgmSeekOffset;
    i32 bgmVolume;
    i32 sfxVolume;
    i32 unkVolume;
};

DIFFABLE_EXTERN(SoundPlayer, g_SoundPlayer)
}; // namespace th08
