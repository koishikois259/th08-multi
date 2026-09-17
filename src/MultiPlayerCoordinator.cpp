#include <windows.h>
#include <string.h>

#include "th_pch.h"
#include "GameManager.hpp"
#include "MultiPlayerCoordinator.hpp"
#include "MultiPlayerRuntime.hpp"
#include "MultiPlayerState.hpp"
#include "SecureRandom.hpp"

namespace th08
{

const u32 TH08_MULTI_BUILD_FINGERPRINT = 0x00030003;
const u16 MULTI_INPUT_TITLE_READY = 0x8000;

namespace
{

void CaptureCurrentSaveProgress(
    MultiNetWelcomePacket::SaveProgress saveProgress[MULTI_NET_SAVE_SHOT_COUNT])
{
    u32 shot;
    u32 difficulty;
    C_ASSERT(MULTI_NET_SAVE_SHOT_COUNT == SHOT_ALL + 1);
    C_ASSERT(MULTI_NET_SAVE_DIFFICULTY_COUNT == MAX_DIFFICULTIES);
    for (shot = 0; shot < MULTI_NET_SAVE_SHOT_COUNT; ++shot)
    {
        for (difficulty = 0; difficulty < MULTI_NET_SAVE_DIFFICULTY_COUNT; ++difficulty)
        {
            saveProgress[shot].clearedWithoutRetries[difficulty] =
                g_GameManager.clrdData[shot].difficultiesClearedWithoutRetries[difficulty];
            saveProgress[shot].clearedWithRetries[difficulty] =
                g_GameManager.clrdData[shot].difficultiesClearedWithRetries[difficulty];
        }
        saveProgress[shot].pendingEndingSkip =
            g_GameManager.clrdData[shot].pendingEndingSkip ? 1 : 0;
    }
}

void ClearCurrentSaveProgress()
{
    u32 shot;
    for (shot = 0; shot < MULTI_NET_SAVE_SHOT_COUNT; ++shot)
    {
        memset(g_GameManager.clrdData[shot].difficultiesClearedWithoutRetries, 0,
               sizeof(g_GameManager.clrdData[shot].difficultiesClearedWithoutRetries));
        memset(g_GameManager.clrdData[shot].difficultiesClearedWithRetries, 0,
               sizeof(g_GameManager.clrdData[shot].difficultiesClearedWithRetries));
        g_GameManager.clrdData[shot].pendingEndingSkip = false;
    }
    g_GameManager.flags.isExtraUnlocked = false;
    g_GameManager.flags.isSpellPracticeUnlocked = false;
    g_GameManager.flags.isExtraUnlockedWithAllTeams = false;
}

} // namespace

MultiPlayerCoordinator g_MultiPlayerCoordinator;

void MultiPlayerLaunchConfig::Load()
{
    char modeText[16];
    i32 value;
    GetPrivateProfileStringA("network", "mode", "disabled", modeText, sizeof(modeText), ".\\th08_multi.ini");
    if (lstrcmpiA(modeText, "host") == 0)
        mode = MULTI_LAUNCH_HOST;
    else if (lstrcmpiA(modeText, "guest") == 0)
        mode = MULTI_LAUNCH_GUEST;
    else
        mode = MULTI_LAUNCH_DISABLED;

    value = GetPrivateProfileIntA("network", "local_port", mode == MULTI_LAUNCH_HOST ? 17708 : 0,
                                  ".\\th08_multi.ini");
    localPort = value >= 0 && value <= 65535 ? static_cast<u16>(value) : 0;
    value = GetPrivateProfileIntA("network", "host_port", 17708, ".\\th08_multi.ini");
    hostPort = value > 0 && value <= 65535 ? static_cast<u16>(value) : 17708;
    value = GetPrivateProfileIntA("network", "input_delay", 3, ".\\th08_multi.ini");
    if (value < MULTI_NET_MIN_INPUT_DELAY)
        value = MULTI_NET_MIN_INPUT_DELAY;
    if (value > MULTI_NET_MAX_INPUT_DELAY)
        value = MULTI_NET_MAX_INPUT_DELAY;
    inputDelay = static_cast<u16>(value);
    selectedTeam = 0;
    GetPrivateProfileStringA("network", "host", "127.0.0.1", hostAddress,
                             sizeof(hostAddress), ".\\th08_multi.ini");
    GetPrivateProfileStringA("network", "bind_address", "0.0.0.0", bindAddress,
                             sizeof(bindAddress), ".\\th08_multi.ini");
}

MultiPlayerCoordinator::MultiPlayerCoordinator()
{
    initialized = false;
    gameplayActive = false;
    gameplaySetupStarted = false;
    gameplayReadyMask = 0;
    gameplayStartFrame = MULTI_NET_INVALID_FRAME;
    selectedTeams[0] = 0;
    selectedTeams[1] = 0;
    capturedSimulationFrame = MULTI_NET_INVALID_FRAME;
    networkFrame = 0;
    displayedState = MULTI_NET_STATE_CLOSED;
    appliedSaveRevision = 0;
    titleSynchronizationActive = false;
    localTitleReady = false;
    localTitleInputArmed = false;
    titleReadyMask = 0;
    titleStartFrame = MULTI_NET_INVALID_FRAME;
}

bool MultiPlayerCoordinator::Initialize()
{
    DWORD nonce;
    DWORD seed;
    MultiNetWelcomePacket::SaveProgress saveProgress[MULTI_NET_SAVE_SHOT_COUNT];
    if (initialized)
        return true;
    config.Load();
    initialized = true;
    if (config.mode == MULTI_LAUNCH_DISABLED)
        return true;
    titleSynchronizationActive = true;
    if (config.mode == MULTI_LAUNCH_GUEST)
        ClearCurrentSaveProgress();
    nonce = 0;
    seed = 0;
    GenerateSecureRandomNonZeroU32(&nonce);
    if (config.mode == MULTI_LAUNCH_HOST)
    {
        GenerateSecureRandomNonZeroU32(&seed);
        CaptureCurrentSaveProgress(saveProgress);
    }
    if (config.mode == MULTI_LAUNCH_HOST)
        return session.OpenHost(config.localPort, config.bindAddress,
                                config.selectedTeam, config.inputDelay,
                                TH08_MULTI_BUILD_FINGERPRINT, nonce, seed, saveProgress);
    return session.OpenGuest(config.localPort, config.bindAddress,
                             config.hostAddress, config.hostPort,
                             config.selectedTeam, config.inputDelay,
                             TH08_MULTI_BUILD_FINGERPRINT, nonce);
}

void MultiPlayerCoordinator::Shutdown()
{
    session.Close(MULTI_NET_DISCONNECT_USER);
    initialized = false;
    gameplayActive = false;
    gameplaySetupStarted = false;
    gameplayReadyMask = 0;
    gameplayStartFrame = MULTI_NET_INVALID_FRAME;
    capturedSimulationFrame = MULTI_NET_INVALID_FRAME;
    networkFrame = 0;
    appliedSaveRevision = 0;
    titleSynchronizationActive = false;
    localTitleReady = false;
    localTitleInputArmed = false;
    titleReadyMask = 0;
    titleStartFrame = MULTI_NET_INVALID_FRAME;
}

void MultiPlayerCoordinator::Pump(u32 nowMilliseconds)
{
    MultiNetSessionState sessionState;
    char title[128];
    if (!initialized)
        Initialize();
    if (config.mode != MULTI_LAUNCH_DISABLED)
    {
        session.Pump(nowMilliseconds);
        sessionState = session.GetState();
        if (config.mode == MULTI_LAUNCH_GUEST && sessionState == MULTI_NET_STATE_CONNECTED &&
            session.GetSaveRevision() != appliedSaveRevision)
            ApplyHostSaveProgress();
        if (sessionState != displayedState && g_Supervisor.hwndGameWindow != NULL)
        {
            if (sessionState == MULTI_NET_STATE_LISTENING)
                wsprintfA(title, "th08-multi v0.32 - HOST waiting on UDP %u", config.localPort);
            else if (sessionState == MULTI_NET_STATE_CONNECTING)
                wsprintfA(title, "th08-multi v0.32 - connecting to %s:%u", config.hostAddress, config.hostPort);
            else if (sessionState == MULTI_NET_STATE_CONNECTED)
                wsprintfA(title, "th08-multi v0.32 - connected (select teams in game / delay %u)",
                          session.GetInputDelay());
            else
                wsprintfA(title, "th08-multi v0.32 - network error %u", session.GetError());
            SetWindowTextA(g_Supervisor.hwndGameWindow, title);
            displayedState = sessionState;
        }
    }
}

bool MultiPlayerCoordinator::IsConfigured() const
{
    return initialized && config.mode != MULTI_LAUNCH_DISABLED;
}

bool MultiPlayerCoordinator::IsConnected() const
{
    return session.GetState() == MULTI_NET_STATE_CONNECTED;
}

bool MultiPlayerCoordinator::IsGameplayActive() const { return gameplayActive; }

bool MultiPlayerCoordinator::IsGameplaySimulationReady() const
{
    return gameplayActive && gameplayReadyMask == 3 &&
           gameplayStartFrame != MULTI_NET_INVALID_FRAME &&
           networkFrame >= gameplayStartFrame;
}

bool MultiPlayerCoordinator::IsSessionFailed() const
{
    return session.GetState() == MULTI_NET_STATE_DISCONNECTED ||
           session.GetState() == MULTI_NET_STATE_ERROR;
}

bool MultiPlayerCoordinator::ShouldSynchronizeInputs() const
{
    return IsConfigured() && IsConnected();
}

bool MultiPlayerCoordinator::IsGuestSaveClient() const
{
    return initialized && config.mode == MULTI_LAUNCH_GUEST;
}

bool MultiPlayerCoordinator::HasHostSaveProgress() const
{
    return IsGuestSaveClient() && session.GetSaveRevision() != 0;
}

bool MultiPlayerCoordinator::ApplyHostSaveProgress()
{
    const MultiNetWelcomePacket::SaveProgress *saveProgress;
    u32 saveRevision;
    u32 shot;
    u32 difficulty;
    if (!HasHostSaveProgress())
        return false;
    saveRevision = session.GetSaveRevision();
    if (saveRevision == appliedSaveRevision)
        return false;
    saveProgress = session.GetSaveProgress();
    for (shot = 0; shot < MULTI_NET_SAVE_SHOT_COUNT; ++shot)
    {
        for (difficulty = 0; difficulty < MULTI_NET_SAVE_DIFFICULTY_COUNT; ++difficulty)
        {
            g_GameManager.clrdData[shot].difficultiesClearedWithoutRetries[difficulty] =
                saveProgress[shot].clearedWithoutRetries[difficulty];
            g_GameManager.clrdData[shot].difficultiesClearedWithRetries[difficulty] =
                saveProgress[shot].clearedWithRetries[difficulty];
        }
        g_GameManager.clrdData[shot].pendingEndingSkip =
            saveProgress[shot].pendingEndingSkip != 0;
    }
    g_GameManager.flags.isExtraUnlocked = g_GameManager.IsExtraUnlocked();
    g_GameManager.flags.isSpellPracticeUnlocked = g_GameManager.IsSpellPracticeUnlocked();
    g_GameManager.flags.isExtraUnlockedWithAllTeams =
        g_GameManager.IsExtraUnlockedWithAllTeams();
    appliedSaveRevision = saveRevision;
    return true;
}

void MultiPlayerCoordinator::BeginTitleSynchronization()
{
    MultiNetWelcomePacket::SaveProgress saveProgress[MULTI_NET_SAVE_SHOT_COUNT];
    if (!IsConfigured())
        return;
    titleSynchronizationActive = true;
    localTitleReady = false;
    localTitleInputArmed = false;
    titleReadyMask = 0;
    titleStartFrame = MULTI_NET_INVALID_FRAME;
    if (config.mode == MULTI_LAUNCH_HOST)
    {
        CaptureCurrentSaveProgress(saveProgress);
        session.UpdateHostSaveProgress(saveProgress);
    }
    else
    {
        ApplyHostSaveProgress();
    }
}

void MultiPlayerCoordinator::MarkLocalTitleReady()
{
    if (titleSynchronizationActive)
        localTitleReady = true;
}

void MultiPlayerCoordinator::EndTitleSynchronization()
{
    titleSynchronizationActive = false;
    localTitleReady = false;
    localTitleInputArmed = false;
    titleReadyMask = 0;
    titleStartFrame = MULTI_NET_INVALID_FRAME;
}

bool MultiPlayerCoordinator::IsTitleInputReady() const
{
    if (!IsConfigured() || !titleSynchronizationActive)
        return true;
    return IsConnected() && titleReadyMask == 3 &&
           titleStartFrame != MULTI_NET_INVALID_FRAME && networkFrame >= titleStartFrame;
}

MultiNetSessionState MultiPlayerCoordinator::GetSessionState() const { return session.GetState(); }
MultiNetSessionError MultiPlayerCoordinator::GetSessionError() const { return session.GetError(); }

void MultiPlayerCoordinator::BeginGameplay(bool newRun, i32 initialLives,
                                            i32 initialBombs, i32 initialPower)
{
    char title[128];
    if (!IsConnected())
        return;
    if ((newRun && !gameplayActive) || !g_MultiPlayerState.IsEnabled())
        g_MultiPlayerState.Reset(true, selectedTeams[0], selectedTeams[1],
                                 initialLives, initialBombs, initialPower);
    gameplayActive = true;
    gameplaySetupStarted = true;
    capturedSimulationFrame = MULTI_NET_INVALID_FRAME;
    if (g_Supervisor.hwndGameWindow != NULL)
    {
        wsprintfA(title, "th08-multi v0.32 - playing (P1 team %u / P2 team %u / delay %u)",
                  selectedTeams[0], selectedTeams[1], session.GetInputDelay());
        SetWindowTextA(g_Supervisor.hwndGameWindow, title);
    }
}

void MultiPlayerCoordinator::PrepareGameplay(i32 initialLives, i32 initialBombs,
                                              i32 initialPower)
{
    if (!IsConnected())
        return;
    g_MultiPlayerState.Reset(true, selectedTeams[0], selectedTeams[1],
                             initialLives, initialBombs, initialPower);
    gameplayActive = true;
    gameplaySetupStarted = false;
    gameplayReadyMask = 0;
    gameplayStartFrame = MULTI_NET_INVALID_FRAME;
    capturedSimulationFrame = MULTI_NET_INVALID_FRAME;
}

void MultiPlayerCoordinator::SetInitialPlayerResources(i32 p1Bombs, i32 p2Bombs, i32 initialPower)
{
    if (!g_MultiPlayerState.IsEnabled())
        return;
    g_MultiPlayerState.GetSlot(MULTI_PLAYER_P1).bombs = p1Bombs;
    g_MultiPlayerState.GetSlot(MULTI_PLAYER_P2).bombs = p2Bombs;
    g_MultiPlayerState.GetSlot(MULTI_PLAYER_P1).power = initialPower;
    g_MultiPlayerState.GetSlot(MULTI_PLAYER_P2).power = initialPower;
}

void MultiPlayerCoordinator::SetSelectedTeams(u8 p1Team, u8 p2Team)
{
    if (p1Team < 4)
        selectedTeams[0] = p1Team;
    if (p2Team < 4)
        selectedTeams[1] = p2Team;
}

bool MultiPlayerCoordinator::AcquireGameplayInputs(u16 localButtons, u16 *p1Buttons, u16 *p2Buttons)
{
    u32 frame;
    u32 hashFrame;
    u32 hash;
    bool synchronizeTitle;
    if (!IsConnected())
        return false;
    frame = networkFrame;
    synchronizeTitle = titleSynchronizationActive && !gameplayActive;
    if (capturedSimulationFrame != frame)
    {
        if (gameplayActive && gameplaySetupStarted && gameplayReadyMask != 3 &&
            g_GameManager.gameplaySetupState == GAMEPLAY_SETUP_COMPLETE)
            localButtons |= TH_BUTTON_RESET;
        if (IsGameplaySimulationReady() &&
            g_MultiPlayerState.frameNumber % 30 == 0)
        {
            hashFrame = g_MultiPlayerState.frameNumber;
            hash = ComputeCurrentMultiPlayerStateHash();
        }
        else
        {
            hashFrame = MULTI_NET_INVALID_FRAME;
            hash = 0;
        }
        if (synchronizeTitle)
        {
            if (!IsTitleInputReady())
                localButtons = localTitleReady ? MULTI_INPUT_TITLE_READY : 0;
            else if (!localTitleInputArmed)
            {
                if (localButtons == 0)
                    localTitleInputArmed = true;
                localButtons = 0;
            }
        }
        if (!session.CaptureLocalInput(frame, localButtons, hashFrame, hash))
            return false;
        capturedSimulationFrame = frame;
    }
    if (!session.TryGetFrameInputs(frame, p1Buttons, p2Buttons))
        return false;
    if (synchronizeTitle)
    {
        if ((*p1Buttons & MULTI_INPUT_TITLE_READY) != 0)
            titleReadyMask |= 1;
        if ((*p2Buttons & MULTI_INPUT_TITLE_READY) != 0)
            titleReadyMask |= 2;
        *p1Buttons &= static_cast<u16>(~MULTI_INPUT_TITLE_READY);
        *p2Buttons &= static_cast<u16>(~MULTI_INPUT_TITLE_READY);
        if (titleReadyMask == 3 && titleStartFrame == MULTI_NET_INVALID_FRAME)
            titleStartFrame = frame + session.GetInputDelay() + 2;
        if (titleStartFrame == MULTI_NET_INVALID_FRAME || frame < titleStartFrame)
        {
            *p1Buttons = 0;
            *p2Buttons = 0;
        }
    }
    if (gameplayActive && gameplaySetupStarted && gameplayReadyMask != 3)
    {
        if ((*p1Buttons & TH_BUTTON_RESET) != 0)
            gameplayReadyMask |= 1;
        if ((*p2Buttons & TH_BUTTON_RESET) != 0)
            gameplayReadyMask |= 2;
        *p1Buttons &= static_cast<u16>(~TH_BUTTON_RESET);
        *p2Buttons &= static_cast<u16>(~TH_BUTTON_RESET);
        if (gameplayReadyMask == 3)
            gameplayStartFrame = frame + session.GetInputDelay() + 2;
    }
    networkFrame++;
    g_MultiPlayerState.GetInput(MULTI_PLAYER_P1).Advance(*p1Buttons);
    g_MultiPlayerState.GetInput(MULTI_PLAYER_P2).Advance(*p2Buttons);
    if (IsGameplaySimulationReady())
    {
        g_MultiPlayerState.frameNumber++;
    }
    capturedSimulationFrame = MULTI_NET_INVALID_FRAME;
    return true;
}

void MultiPlayerCoordinator::EndGameplay()
{
    gameplayActive = false;
    gameplaySetupStarted = false;
    gameplayReadyMask = 0;
    gameplayStartFrame = MULTI_NET_INVALID_FRAME;
    capturedSimulationFrame = MULTI_NET_INVALID_FRAME;
}

u8 MultiPlayerCoordinator::GetHostTeam() const { return selectedTeams[0]; }
u8 MultiPlayerCoordinator::GetGuestTeam() const { return selectedTeams[1]; }
u8 MultiPlayerCoordinator::GetLocalSlot() const { return session.GetLocalSlot(); }
u32 MultiPlayerCoordinator::GetRandomSeed() const { return session.GetRandomSeed(); }
u32 MultiPlayerCoordinator::GetDesyncFrame() const { return session.GetDesyncFrame(); }
u32 MultiPlayerCoordinator::GetLatestRemoteFrame() const { return session.GetLatestRemoteFrame(); }
u32 MultiPlayerCoordinator::GetLatestAcknowledgedFrame() const
{
    return session.GetLatestAcknowledgedFrame();
}

} // namespace th08
