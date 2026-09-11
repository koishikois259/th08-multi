#pragma once

#include "inttypes.hpp"

namespace th08
{

enum MultiPlayerSlot
{
    MULTI_PLAYER_P1 = 0,
    MULTI_PLAYER_P2 = 1,
    MULTI_PLAYER_COUNT = 2,
    MULTI_PLAYER_NONE = -1,
};

enum MultiPlayerPresence
{
    MULTI_PLAYER_INACTIVE = 0,
    MULTI_PLAYER_PHYSICAL = 1,
    MULTI_PLAYER_SPIRIT = 2,
};

enum MultiPlayerReviveResult
{
    MULTI_REVIVE_NONE = 0,
    MULTI_REVIVE_PROGRESS = 1,
    MULTI_REVIVE_CANCELLED = 2,
    MULTI_REVIVE_COMPLETED = 3,
};

struct MultiPlayerInputFrame
{
    u16 current;
    u16 previous;

    void Reset();
    void Advance(u16 buttons);
    bool IsPressed(u16 buttons) const;
    bool WasPressed(u16 buttons) const;
};

struct MultiPlayerPosition
{
    f32 x;
    f32 y;
};

struct MultiPlayerSlotState
{
    i32 lives;
    i32 bombs;
    i32 power;
    i32 youkaiGauge;
    i32 graze;
    i32 grazeInStage;
    i32 deaths;
    i32 bombsUsed;
    i32 reviveProgressFrames;
    MultiPlayerPosition spiritVelocity;
    u32 team;
    MultiPlayerPresence presence;
    MultiPlayerInputFrame input;

    void Reset(u32 selectedTeam, i32 initialLives, i32 initialBombs, i32 initialPower);
};

struct MultiPlayerSharedSnapshot
{
    u32 score;
    u32 pointItems;
    u32 pointItemValue;
    u32 timeOrbs;
    u32 totalTimeOrbs;
    u32 rngSeed;
    u32 activeEnemies;
    u32 activeBullets;
    i32 bossLife;
    i32 bossPhase;
};

class MultiPlayerState
{
  public:
    enum
    {
        REVIVE_HOLD_FRAMES = 90,
    };

    MultiPlayerState();

    void Reset(bool enabled, u32 p1Team, u32 p2Team, i32 initialLives, i32 initialBombs, i32 initialPower);
    void ResetForContinue(i32 initialLives, i32 p1InitialBombs, i32 p2InitialBombs, i32 initialPower);
    bool IsEnabled() const;
    MultiPlayerSlotState &GetSlot(MultiPlayerSlot slot);
    const MultiPlayerSlotState &GetSlot(MultiPlayerSlot slot) const;
    MultiPlayerInputFrame &GetInput(MultiPlayerSlot slot);
    const MultiPlayerInputFrame &GetInput(MultiPlayerSlot slot) const;
    bool IsPhysical(MultiPlayerSlot slot) const;
    bool IsSpirit(MultiPlayerSlot slot) const;
    bool BothPlayersUnableToContinue() const;
    void ResetStageCounters();
    i32 GetCombinedGrazeInStage() const;

    MultiPlayerSlot ResolveNearestPhysicalPlayer(
        const MultiPlayerPosition &target,
        const MultiPlayerPosition positions[MULTI_PLAYER_COUNT]) const;

    void EnterSpirit(MultiPlayerSlot slot);
    void UpdateSpiritPosition(
        MultiPlayerSlot slot,
        MultiPlayerPosition *position,
        f32 left,
        f32 top,
        f32 right,
        f32 bottom);
    MultiPlayerReviveResult UpdateRevival(
        MultiPlayerSlot rescuer,
        bool playersOverlap,
        bool focusHeld,
        bool shooting);

    u32 ComputeStateHash(
        const MultiPlayerPosition positions[MULTI_PLAYER_COUNT],
        const MultiPlayerSharedSnapshot &shared) const;

    u32 frameNumber;

  private:
    bool enabled;
    MultiPlayerSlotState slots[MULTI_PLAYER_COUNT];
};

extern MultiPlayerState g_MultiPlayerState;

} // namespace th08
