#include "SoundPlayer.hpp"

#include "Global.hpp"
#include "Supervisor.hpp"
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
    "se_plst00.wav",   "se_enep00.wav",  "se_pldead00.wav", "se_power0.wav",   "se_power1.wav",   "se_tan00.wav",
    "se_tan01.wav",    "se_tan02.wav",   "se_ok00.wav",     "se_cancel00.wav", "se_select00.wav", "se_gun00.wav",
    "se_cat00.wav",    "se_lazer00.wav", "se_lazer01.wav",  "se_enep01.wav",   "se_nep00.wav",    "se_damage00.wav",
    "se_item00.wav",   "se_kira00.wav",  "se_kira01.wav",   "se_kira02.wav",   "se_extend.wav",   "se_timeout.wav",
    "se_graze.wav",    "se_powerup.wav", "se_pause.wav",    "se_cardget.wav",  "se_option.wav",   "se_damage01.wav",
    "se_timeout2.wav", "se_opshow.wav",  "se_ophide.wav",   "se_invalid.wav",  "se_slash.wav",    "se_item01.wav",
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
        this->soundQueue[i] = -1;
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
    if (FAILED(this->initSoundBuffer->Lock(0, BGM_BUFFER_SIZE, &audioBuffer1Start, &audioBuffer1Len, &audioBuffer2Start,
                                           &audioBuffer2Len, 0)))
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

#pragma var_order(pos, i, buffer)
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

#pragma var_order(sFDCursor, dsBuffer, wavDataPtr, formatSize, audioPtr2, audioSize2, audioSize1, audioPtr1,           \
                  soundFileData, wavData, fileSize)
ZunResult SoundPlayer::LoadSound(i32 idx, char *path)
{
    u8 *soundFileData;
    u8 *sFDCursor;
    i32 fileSize;
    WAVEFORMATEX *wavDataPtr;
    WAVEFORMATEX *audioPtr1;
    WAVEFORMATEX *audioPtr2;
    DWORD audioSize1;
    DWORD audioSize2;
    WAVEFORMATEX wavData;
    i32 formatSize;
    DSBUFFERDESC dsBuffer;

    if (this->manager == NULL)
    {
        return ZUN_SUCCESS;
    }
    SAFE_RELEASE(this->soundBuffers[idx]);
    soundFileData = (u8 *)FileSystem::OpenFile(path, NULL, FALSE);
    sFDCursor = soundFileData;
    if (sFDCursor == NULL)
    {
        return ZUN_ERROR;
    }
    if (strncmp((char *)sFDCursor, "RIFF", 4))
    {
        g_GameErrorContext.Log(TH_ERR_NOT_A_WAV_FILE, path);
        g_ZunMemory.Free(soundFileData);
        return ZUN_ERROR;
    }
    sFDCursor += 4;

    fileSize = *(i32 *)sFDCursor;
    sFDCursor += 4;

    if (strncmp((char *)sFDCursor, "WAVE", 4))
    {
        g_GameErrorContext.Log(TH_ERR_NOT_A_WAV_FILE, path);
        g_ZunMemory.Free(soundFileData);
        return ZUN_ERROR;
    }
    sFDCursor += 4;
    wavDataPtr = GetWavFormatData(sFDCursor, "fmt ", &formatSize, fileSize - 12);
    if (wavDataPtr == NULL)
    {
        g_GameErrorContext.Log(TH_ERR_NOT_A_WAV_FILE, path);
        g_ZunMemory.Free(soundFileData);
        return ZUN_ERROR;
    }
    wavData = *wavDataPtr;

    wavDataPtr = GetWavFormatData(sFDCursor, "data", &formatSize, fileSize - 12);
    if (wavDataPtr == NULL)
    {
        g_GameErrorContext.Log(TH_ERR_NOT_A_WAV_FILE, path);
        g_ZunMemory.Free(soundFileData);
        return ZUN_ERROR;
    }
    ZeroMemory(&dsBuffer, sizeof(dsBuffer));
    dsBuffer.dwSize = sizeof(dsBuffer);
    dsBuffer.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_LOCSOFTWARE;
    dsBuffer.dwBufferBytes = formatSize;
    dsBuffer.lpwfxFormat = &wavData;
    if (FAILED(this->dsoundHdl->CreateSoundBuffer(&dsBuffer, &this->soundBuffers[idx], NULL)))
    {
        g_ZunMemory.Free(soundFileData);
        return ZUN_ERROR;
    }
    if (FAILED(soundBuffers[idx]->Lock(0, formatSize, (LPVOID *)&audioPtr1, (LPDWORD)&audioSize1, (LPVOID *)&audioPtr2,
                                       (LPDWORD)&audioSize2, NULL)))
    {
        g_ZunMemory.Free(soundFileData);
        return ZUN_ERROR;
    }
    CopyMemory(audioPtr1, wavDataPtr, audioSize1);
    if (audioSize2 != 0)
    {
        CopyMemory(audioPtr2, (i8 *)wavDataPtr + audioSize1, audioSize2);
    }
    soundBuffers[idx]->Unlock((LPVOID *)audioPtr1, audioSize1, (LPVOID *)audioPtr2, audioSize2);
    g_ZunMemory.Free(soundFileData);
    return ZUN_SUCCESS;
}

WAVEFORMATEX *SoundPlayer::GetWavFormatData(u8 *soundData, char *formatString, i32 *formatSize,
                                            u32 fileSizeExcludingFormat)
{
    while (fileSizeExcludingFormat > 0)
    {
        *formatSize = *(i32 *)(soundData + 4);
        if (strncmp((char *)soundData, formatString, 4) == 0)
        {
            return (WAVEFORMATEX *)(soundData + 8);
        }
        fileSizeExcludingFormat -= (*formatSize + 8);
        soundData += *formatSize + 8;
    }
    return NULL;
}

ZunResult SoundPlayer::LoadFmt(char *path)
{
    this->bgmFmtData = (ThBgmFormat *)FileSystem::OpenFile(path, NULL, FALSE);
    return this->bgmFmtData != NULL ? ZUN_SUCCESS : ZUN_ERROR;
}

#pragma var_order(notifySize, fmtData, res, numSamplesPerSec, blockAlign)
ZunResult SoundPlayer::StartBGM(char *path)
{
    HRESULT res;
    ThBgmFormat *fmtData;
    DWORD blockAlign;
    DWORD numSamplesPerSec;
    DWORD notifySize;

    strcpy(this->currentBgmFileName, path);

    if (this->manager == NULL)
        return ZUN_ERROR;

    if (this->dsoundHdl == NULL)
        return ZUN_ERROR;

    utils::DebugPrint("Streming BGM Start\r\n");
    this->StopBGM();

    fmtData = this->bgmFmtData;
    blockAlign = fmtData->format.nBlockAlign;
    numSamplesPerSec = fmtData->format.nSamplesPerSec;
    notifySize = numSamplesPerSec * 4 * blockAlign / BGM_WAV_BITS_PER_SAMPLE;
    notifySize -= (notifySize % blockAlign);
    this->bgmUpdateEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    this->bgmThreadHandle =
        CreateThread(NULL, 0, SoundPlayer::BGMPlayerThread, g_Supervisor.hwndGameWindow, 0, &this->bgmThreadId);
    res = this->manager->CreateStreaming(&this->bgm, path, DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLPOSITIONNOTIFY,
                                         GUID_NULL, 16, notifySize, this->bgmUpdateEvent, fmtData);
    if (FAILED(res))
    {
        utils::DebugPrint(TH_ERR_SOUNDPLAYER_FAILED_TO_CREATE_BGM_SOUND_BUFFER);
        return ZUN_ERROR;
    }
    return ZUN_SUCCESS;
}

void SoundPlayer::PlaySoundByIdx(SoundIdx idx, i32 unused)
{
    i32 unk;
    i32 i;

    unk = g_SoundBufferIdxVol[idx].unk;
    for (i = 0; i < SOUND_QUEUE_LENGTH; i++)
    {
        if (this->soundQueue[i] < 0)
            break;

        if (this->soundQueue[i] == idx)
        {
            if (this->unk650[i] < 0x80)
                this->soundQueuePanData[i][this->unk650[i]++] = unused;

            return;
        }
    }

    if (i >= SOUND_QUEUE_LENGTH)
        return;

    this->soundQueue[i] = idx;
    this->unk408[idx] = unk;
    this->soundQueuePanData[i][0] = unused;
    this->unk650[i]++;
}

void SoundPlayer::PlaySoundPositionedByIdx(SoundIdx idx, f32 pan)
{
    i32 unk;
    i32 panAsInt;
    i32 i;

    unk = g_SoundBufferIdxVol[idx].unk;
    panAsInt = ((pan - 192) * 1000) / 192;

    for (i = 0; i < SOUND_QUEUE_LENGTH; i++)
    {
        if (this->soundQueue[i] < 0)
            break;

        if (this->soundQueue[i] == idx)
        {
            if (this->unk650[i] < 0x80)
                this->soundQueuePanData[i][this->unk650[i]++] = panAsInt;

            return;
        }
    }

    if (i >= SOUND_QUEUE_LENGTH)
        return;

    this->soundQueue[i] = idx;
    this->unk408[idx] = unk;
    this->soundQueuePanData[i][0] = panAsInt;
    this->unk650[i]++;
}

#pragma var_order(msg, looped, lpThreadParameterCopy, waitObj, res, stopped)
DWORD WINAPI SoundPlayer::BGMPlayerThread(LPVOID lpThreadParameter)
{
    DWORD waitObj;
    MSG msg;
    u32 stopped;
    u32 looped;
    LPVOID lpThreadParameterCopy;
    HRESULT res;

    lpThreadParameterCopy = lpThreadParameter;
    stopped = false;
    looped = true;
    while (!stopped)
    {
        waitObj = MsgWaitForMultipleObjects(1, &g_SoundPlayer.bgmUpdateEvent, FALSE, INFINITE, QS_ALLEVENTS);
        if (g_SoundPlayer.bgm == NULL)
        {
            stopped = true;
        }
        switch (waitObj)
        {
        case 0:
            if (g_SoundPlayer.bgm != NULL && g_SoundPlayer.bgm->m_bIsPlaying)
            {
                g_SoundPlayer.bgm->m_bIsLocked = TRUE;
                res = g_SoundPlayer.bgm->HandleWaveStreamNotification(looped);
                g_SoundPlayer.bgm->m_bIsLocked = FALSE;
            }
            break;
        case 1:
            while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE) != 0)
            {
                if (msg.message == WM_QUIT)
                {
                    stopped = true;
                }
            }
            break;
        }
    }
    utils::DebugPrint(TH_DBG_SOUNDPLAYER_BGM_THREAD_TERMINATED);
    return 0;
}

void SoundPlayer::QueueCommand(i32 opcode, i32 arg, char *unused)
{
    i32 i;

    for (i = 0; i < ARRAY_SIZE_SIGNED(this->commandQueue) - 1; i++)
    {
        if (this->commandQueue[i].opcode != 0)
            continue;

        this->commandQueue[i].opcode = opcode;
        this->commandQueue[i].arg1 = arg;
        strcpy(this->commandQueue[i].string, unused);
        this->commandQueue[i].arg2 = 0;

        break;
    }

    utils::DebugPrint("Sound Que Add %d\r\n", opcode);
    return;
}

SoundPlayer::SoundPlayer()
{
    ZeroMemory(this, sizeof(SoundPlayer));
    for (i32 i = 0; i < NUM_SOUND_BUFFERS; i++)
    {
        this->unk408[i] = -1;
    }
}

void SoundPlayer::FreePreloadedBGM(i32 idx)
{
    if (this->unk1ec0[idx] != NULL)
    {
        g_ZunMemory.Free(this->unk1ec0[idx]);
        this->unk1ec0[idx] = NULL;
    }
}

void SoundPlayer::FadeOut(f32 seconds)
{
    if (this->bgm != NULL)
    {
        this->bgm->FadeOut(seconds);
    }
}
}; // namespace th08
