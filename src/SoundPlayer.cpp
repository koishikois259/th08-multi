#include "SoundPlayer.hpp"

#include "Global.hpp"
#include "dxutil.hpp"
#include "i18n.hpp"
#include "utils.hpp"

namespace th08
{

#define BGM_BUFFER_SIZE 0x8000
#define BGM_WAV_NUM_CHANNELS 2
#define BGM_WAV_BITS_PER_SAMPLE 16
#define BGM_WAV_BLOCK_ALIGN (BGM_WAV_BITS_PER_SAMPLE / 8 * BGM_WAV_NUM_CHANNELS)
#define BGM_WAV_SAMPLES_PER_SEC 44100

DIFFABLE_STATIC_ARRAY_ASSIGN(SoundBufferIdxVolume, 46, g_SoundBufferIdxVol) = {
    {0, -1900, 0},   {0, -2100, 0},   {1, -1200, 5},    {1, -1500, 5},   {2, -1100, 100}, {3, -700, 100},
    {4, -700, 100},  {5, -1900, 50},  {6, -2200, 50},   {7, -2400, 50},  {8, -1100, 100}, {9, -1100, 100},
    {10, -1500, 10}, {11, -1500, 10}, {12, -1000, 100}, {5, -1100, 50},  {13, -1300, 50}, {14, -1400, 50},
    {15, -900, 100}, {16, -400, 100}, {17, -880, 0},    {18, -1500, 0},  {5, -300, 20},   {6, -1800, 20},
    {7, -1800, 20},  {19, -1100, 50}, {20, -1300, 50},  {21, -1500, 50}, {22, -500, 140}, {23, -500, 100},
    {24, -1100, 20}, {25, -800, 90},  {24, -1200, 20},  {19, -500, 50},  {26, -800, 100}, {27, -800, 100},
    {28, -800, 100}, {29, -700, 0},   {30, -300, 100},  {31, -800, 100}, {32, -800, 100}, {33, -200, 100},
    {34, 0, 100},    {34, -600, 100}, {35, -800, 0},    {8, -100, 100},
};
DIFFABLE_STATIC_ARRAY_ASSIGN(char *, 36, g_SFXList) = {
    "se_plst00.wav", "se_enep00.wav",   "se_pldead00.wav", "se_power0.wav",
    "se_power1.wav", "se_tan00.wav",    "se_tan01.wav",    "se_tan02.wav",
    "se_ok00.wav",   "se_cancel00.wav", "se_select00.wav", "se_gun00.wav",
    "se_cat00.wav",  "se_lazer00.wav",  "se_lazer01.wav",  "se_enep01.wav",
    "se_nep00.wav",  "se_damage00.wav", "se_item00.wav",   "se_kira00.wav",
    "se_kira01.wav", "se_kira02.wav",   "se_extend.wav",   "se_timeout.wav",
    "se_graze.wav",  "se_powerup.wav",  "se_pause.wav",    "se_cardget.wav",
    "se_option.wav", "se_damage01.wav", "se_timeout2.wav", "se_opshow.wav",
    "se_ophide.wav", "se_invalid.wav",  "se_slash.wav",    "se_item01.wav",
};
DIFFABLE_STATIC(SoundPlayer, g_SoundPlayer)

#pragma var_order(bufDesc, audioBuffer2Start, audioBuffer2Len, audioBuffer1Len, audioBuffer1Start, wavFormat)
ZunResult SoundPlayer::InitializeDSound(HWND gameWindow)
{
    DSBUFFERDESC bufDesc;
    WAVEFORMATEX wavFormat;
    LPVOID audioBuffer1Start;
    DWORD audioBuffer1Len;
    LPVOID audioBuffer2Start;
    DWORD audioBuffer2Len;

    ZeroMemory(this, sizeof(SoundPlayer));
    
    for (i32 i = 0; i < NUM_SOUND_BUFFERS; i++)
    {
        this->unk408[i] = -1;
    }
    for (i32 i = 0; i < SOUND_QUEUE_LENGTH; i++)
    {
        this->soundBuffersToPlay[i] = -1;
    }

    this->manager = new CSoundManager();
    if (FAILED(this->manager->Initialize(gameWindow, 2, 2, BGM_WAV_SAMPLES_PER_SEC, BGM_WAV_BITS_PER_SAMPLE)))
    {
        g_GameErrorContext.Log(TH_ERR_SOUNDPLAYER_FAILED_TO_INITIALIZE_OBJECT);
        SAFE_DELETE(this->manager);
        return ZUN_ERROR;
    }

    this->dsoundHdl = this->manager->GetDirectSound();
    this->bgmThreadHandle = NULL;
    ZeroMemory(&bufDesc, sizeof(DSBUFFERDESC));
    bufDesc.dwSize = sizeof(DSBUFFERDESC);
    bufDesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_LOCSOFTWARE;
    bufDesc.dwBufferBytes = BGM_BUFFER_SIZE;
    ZeroMemory(&wavFormat, sizeof(WAVEFORMATEX));
    wavFormat.cbSize = 0;
    wavFormat.wFormatTag = WAVE_FORMAT_PCM;
    wavFormat.nChannels = BGM_WAV_NUM_CHANNELS;
    wavFormat.nSamplesPerSec = BGM_WAV_SAMPLES_PER_SEC;
    wavFormat.nAvgBytesPerSec = BGM_WAV_SAMPLES_PER_SEC * sizeof(INT16) * BGM_WAV_NUM_CHANNELS;
    wavFormat.nBlockAlign = BGM_WAV_BLOCK_ALIGN;
    wavFormat.wBitsPerSample = BGM_WAV_BITS_PER_SAMPLE;
    bufDesc.lpwfxFormat = &wavFormat;
    if (FAILED(this->dsoundHdl->CreateSoundBuffer(&bufDesc, &this->initSoundBuffer, NULL)))
    {
        return ZUN_ERROR;
    }
    if (FAILED(this->initSoundBuffer->Lock(0, BGM_BUFFER_SIZE, &audioBuffer1Start, &audioBuffer1Len,
                                           &audioBuffer2Start, &audioBuffer2Len, 0)))
    {
        return ZUN_ERROR;
    }

    ZeroMemory(audioBuffer1Start, BGM_BUFFER_SIZE);
    this->initSoundBuffer->Unlock(audioBuffer1Start, audioBuffer1Len, audioBuffer2Start, audioBuffer2Len);
    this->initSoundBuffer->Play(0, 0, 1);
    this->bgmVolume = 100;
    this->sfxVolume = 100;
    /* 4 times per second */
    SetTimer(gameWindow, 0, 250, NULL);
    this->gameWindow = gameWindow;
    g_GameErrorContext.Log(TH_DBG_SOUNDPLAYER_INIT_SUCCESS);
    return ZUN_SUCCESS;
}

ZunResult SoundPlayer::Release()
{
    i32 i;

    if (this->bgmFmtData != NULL)
    {
        g_ZunMemory.Free(this->bgmFmtData);
    }
    for (i = 0; i < NUM_SOUND_BUFFERS; i++)
    {
        SAFE_RELEASE(this->duplicateSoundBuffers[i]);
        SAFE_RELEASE(this->soundBuffers[i]);
    }
    if (this->manager == NULL)
    {
        return ZUN_SUCCESS;
    }
    KillTimer(this->gameWindow, 1);
    this->StopBGM();
    this->dsoundHdl = NULL;
    this->initSoundBuffer->Stop();
    SAFE_RELEASE(this->initSoundBuffer);
    SAFE_DELETE(this->bgm);
    SAFE_DELETE(this->manager);
    for (i = 0; i < NUM_BGM_SLOTS; i++)
    {
        this->FreePreloadedBGM(i);
    }
    return ZUN_SUCCESS;
}

#pragma var_order (pos, i, buffer)
i32 SoundPlayer::GetFmtIndexByName(char *name)
{
    char *pos;
    i32 i = 0;
    char buffer[128];

    pos = strrchr(name, '/');
    if (pos == NULL)
    {
        pos = strrchr(name, '\\');
    }
    if (pos == NULL)
    {
        strcpy(buffer, name);
    }
    else
    {
        strcpy(buffer, pos + 1);
    }
    while (this->bgmFmtData[i].name[0] != '\0')
    {
        if (strcmp(this->bgmFmtData[i].name, buffer) == 0)
        {
            break;
        }
        i++;
    }
    if (this->bgmFmtData[i].name[0] == '\0')
    {
        i = 0;
    }
    return i;
}

};
