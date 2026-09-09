#include <windows.h>
#include <string.h>

#include "th_pch.h"
#include "MultiPlayerCoordinator.hpp"
#include "MultiPlayerRuntime.hpp"
#include "MultiPlayerState.hpp"

namespace th08
{

const u32 TH08_MULTI_BUILD_FINGERPRINT = 0x00010001;

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
    value = GetPrivateProfileIntA("player", "team", 0, ".\\th08_multi.ini");
    selectedTeam = value >= 0 && value < 4 ? static_cast<u8>(value) : 0;
    GetPrivateProfileStringA("network", "host", "127.0.0.1", hostAddress,
                             sizeof(hostAddress), ".\\th08_multi.ini");
}

MultiPlayerCoordinator::MultiPlayerCoordinator()
{
    initialized = false;
    gameplayActive = false;
    capturedSimulationFrame = MULTI_NET_INVALID_FRAME;
    networkFrame = 0;
}

bool MultiPlayerCoordinator::Initialize()
{
    u32 nonce;
    u32 seed;
    if (initialized)
        return true;
    config.Load();
    initialized = true;
    if (config.mode == MULTI_LAUNCH_DISABLED)
        return true;
    nonce = GetTickCount() ^ (GetCurrentProcessId() * 0x45D9F3B);
    seed = nonce ^ 0xA5366B4D;
    if (config.mode == MULTI_LAUNCH_HOST)
        return session.OpenHost(config.localPort, config.selectedTeam, config.inputDelay,
                                TH08_MULTI_BUILD_FINGERPRINT, nonce, seed);
    return session.OpenGuest(config.localPort, config.hostAddress, config.hostPort,
                             config.selectedTeam, config.inputDelay,
                             TH08_MULTI_BUILD_FINGERPRINT, nonce);
}

void MultiPlayerCoordinator::Shutdown()
{
    session.Close(MULTI_NET_DISCONNECT_USER);
    initialized = false;
    gameplayActive = false;
    capturedSimulationFrame = MULTI_NET_INVALID_FRAME;
    networkFrame = 0;
}

void MultiPlayerCoordinator::Pump(u32 nowMilliseconds)
{
    if (!initialized)
        Initialize();
    if (config.mode != MULTI_LAUNCH_DISABLED)
        session.Pump(nowMilliseconds);
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

bool MultiPlayerCoordinator::IsSessionFailed() const
{
    return session.GetState() == MULTI_NET_STATE_DISCONNECTED ||
           session.GetState() == MULTI_NET_STATE_ERROR;
}

bool MultiPlayerCoordinator::ShouldSynchronizeInputs() const
{
    return IsConfigured() && IsConnected();
}

MultiNetSessionState MultiPlayerCoordinator::GetSessionState() const { return session.GetState(); }
MultiNetSessionError MultiPlayerCoordinator::GetSessionError() const { return session.GetError(); }

void MultiPlayerCoordinator::BeginGameplay(bool newRun, i32 initialLives,
                                            i32 initialBombs, i32 initialPower)
{
    if (!IsConnected())
        return;
    if (newRun || !g_MultiPlayerState.IsEnabled())
        g_MultiPlayerState.Reset(true, session.GetHostTeam(), session.GetGuestTeam(),
                                 initialLives, initialBombs, initialPower);
    gameplayActive = true;
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

bool MultiPlayerCoordinator::AcquireGameplayInputs(u16 localButtons, u16 *p1Buttons, u16 *p2Buttons)
{
    u32 frame;
    u32 hashFrame;
    u32 hash;
    if (!IsConnected())
        return false;
    frame = networkFrame;
    if (capturedSimulationFrame != frame)
    {
        if (gameplayActive && g_MultiPlayerState.frameNumber % 30 == 0)
        {
            hashFrame = g_MultiPlayerState.frameNumber;
            hash = ComputeCurrentMultiPlayerStateHash();
        }
        else
        {
            hashFrame = MULTI_NET_INVALID_FRAME;
            hash = 0;
        }
        if (!session.CaptureLocalInput(frame, localButtons, hashFrame, hash))
            return false;
        capturedSimulationFrame = frame;
    }
    if (!session.TryGetFrameInputs(frame, p1Buttons, p2Buttons))
        return false;
    networkFrame++;
    if (gameplayActive)
    {
        g_MultiPlayerState.GetInput(MULTI_PLAYER_P1).Advance(*p1Buttons);
        g_MultiPlayerState.GetInput(MULTI_PLAYER_P2).Advance(*p2Buttons);
        g_MultiPlayerState.frameNumber++;
    }
    capturedSimulationFrame = MULTI_NET_INVALID_FRAME;
    return true;
}

void MultiPlayerCoordinator::EndGameplay()
{
    gameplayActive = false;
    capturedSimulationFrame = MULTI_NET_INVALID_FRAME;
}

u8 MultiPlayerCoordinator::GetHostTeam() const { return session.GetHostTeam(); }
u8 MultiPlayerCoordinator::GetGuestTeam() const { return session.GetGuestTeam(); }
u8 MultiPlayerCoordinator::GetLocalSlot() const { return session.GetLocalSlot(); }
u32 MultiPlayerCoordinator::GetRandomSeed() const { return session.GetRandomSeed(); }

} // namespace th08
