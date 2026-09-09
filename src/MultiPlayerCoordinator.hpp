#pragma once

#include "MultiNetSession.hpp"

namespace th08
{

enum MultiPlayerLaunchMode
{
    MULTI_LAUNCH_DISABLED = 0,
    MULTI_LAUNCH_HOST = 1,
    MULTI_LAUNCH_GUEST = 2,
};

struct MultiPlayerLaunchConfig
{
    MultiPlayerLaunchMode mode;
    u16 localPort;
    u16 hostPort;
    u16 inputDelay;
    u8 selectedTeam;
    char hostAddress[64];

    void Load();
};

class MultiPlayerCoordinator
{
  public:
    MultiPlayerCoordinator();

    bool Initialize();
    void Shutdown();
    void Pump(u32 nowMilliseconds);
    bool IsConfigured() const;
    bool IsConnected() const;
    bool IsGameplayActive() const;
    bool IsSessionFailed() const;
    bool ShouldSynchronizeInputs() const;
    MultiNetSessionState GetSessionState() const;
    MultiNetSessionError GetSessionError() const;

    void BeginGameplay(bool newRun, i32 initialLives, i32 initialBombs, i32 initialPower);
    void SetInitialPlayerResources(i32 p1Bombs, i32 p2Bombs, i32 initialPower);
    bool AcquireGameplayInputs(u16 localButtons, u16 *p1Buttons, u16 *p2Buttons);
    void EndGameplay();

    u8 GetHostTeam() const;
    u8 GetGuestTeam() const;
    u8 GetLocalSlot() const;
    u32 GetRandomSeed() const;
    u32 GetDesyncFrame() const;
    u32 GetLatestRemoteFrame() const;
    u32 GetLatestAcknowledgedFrame() const;

  private:
    MultiPlayerLaunchConfig config;
    MultiNetSession session;
    u32 capturedSimulationFrame;
    u32 networkFrame;
    bool initialized;
    bool gameplayActive;
    MultiNetSessionState displayedState;
};

extern MultiPlayerCoordinator g_MultiPlayerCoordinator;

} // namespace th08
