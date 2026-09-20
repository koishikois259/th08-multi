#pragma once

#include "MultiNetSession.hpp"

namespace th08
{

enum MultiPlayerLaunchMode
{
    MULTI_LAUNCH_DISABLED = 0,
    MULTI_LAUNCH_HOST = 1,
    MULTI_LAUNCH_GUEST = 2,
    MULTI_LAUNCH_LOCAL = 3,
};

struct MultiPlayerLaunchConfig
{
    MultiPlayerLaunchMode mode;
    u16 localPort;
    u16 hostPort;
    u16 inputDelay;
    u8 selectedTeam;
    char hostAddress[64];
    char bindAddress[64];

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
    bool IsLocalPlay() const;
    bool IsGameplayActive() const;
    bool IsGameplaySimulationReady() const;
    bool IsSessionFailed() const;
    bool ShouldSynchronizeInputs() const;
    bool IsGuestSaveClient() const;
    bool HasHostSaveProgress() const;
    bool ApplyHostSaveProgress();
    void BeginTitleSynchronization();
    void MarkLocalTitleReady();
    void EndTitleSynchronization();
    bool IsTitleInputReady() const;
    MultiNetSessionState GetSessionState() const;
    MultiNetSessionError GetSessionError() const;

    void PrepareGameplay(i32 initialLives, i32 initialBombs, i32 initialPower);
    void BeginGameplay(bool newRun, i32 initialLives, i32 initialBombs, i32 initialPower);
    void SetInitialPlayerResources(i32 p1Bombs, i32 p2Bombs, i32 initialPower);
    void SetSelectedTeams(u8 p1Team, u8 p2Team);
    bool AcquireGameplayInputs(u16 localButtons, u16 *p1Buttons, u16 *p2Buttons);
    u16 GetLocalInput();
    void EndGameplay();

    u8 GetHostTeam() const;
    u8 GetGuestTeam() const;
    u8 GetLocalSlot() const;
    u8 GetInitialLives() const;
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
    bool gameplaySetupStarted;
    bool gameplayRngSynchronized;
    u8 gameplayReadyMask;
    u32 gameplayStartFrame;
    u8 selectedTeams[2];
    MultiNetSessionState displayedState;
    u32 appliedSaveRevision;
    bool titleSynchronizationActive;
    bool localTitleReady;
    bool localTitleInputArmed;
    u8 titleReadyMask;
    u32 titleStartFrame;
    u32 localRandomSeed;
    bool localInputFailed;
};

extern MultiPlayerCoordinator g_MultiPlayerCoordinator;

} // namespace th08
