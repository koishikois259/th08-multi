#include "th_pch.h"
#include "MultiPlayerState.hpp"

namespace th08
{

MultiPlayerState g_MultiPlayerState;

namespace
{

u32 MixHash(u32 hash, u32 value)
{
    hash ^= value & 0xFF;
    hash *= 16777619u;
    hash ^= (value >> 8) & 0xFF;
    hash *= 16777619u;
    hash ^= (value >> 16) & 0xFF;
    hash *= 16777619u;
    hash ^= (value >> 24) & 0xFF;
    hash *= 16777619u;
    return hash;
}

u32 FloatBits(f32 value)
{
    union
    {
        f32 floatValue;
        u32 bits;
    } conversion;
    conversion.floatValue = value;
    return conversion.bits;
}

f32 DistanceSquared(const MultiPlayerPosition &left, const MultiPlayerPosition &right)
{
    f32 x = left.x - right.x;
    f32 y = left.y - right.y;
    return x * x + y * y;
}

MultiPlayerSlot OtherSlot(MultiPlayerSlot slot)
{
    return slot == MULTI_PLAYER_P1 ? MULTI_PLAYER_P2 : MULTI_PLAYER_P1;
}

} // namespace

void MultiPlayerInputFrame::Reset()
{
    current = 0;
    previous = 0;
}

void MultiPlayerInputFrame::Advance(u16 buttons)
{
    previous = current;
    current = buttons;
}

bool MultiPlayerInputFrame::IsPressed(u16 buttons) const
{
    return (current & buttons) != 0;
}

bool MultiPlayerInputFrame::WasPressed(u16 buttons) const
{
    return (current & buttons) != 0 && (previous & buttons) == 0;
}

void MultiPlayerSlotState::Reset(u32 selectedTeam, i32 initialLives, i32 initialBombs, i32 initialPower)
{
    lives = initialLives;
    bombs = initialBombs;
    power = initialPower;
    youkaiGauge = 0;
    graze = 0;
    grazeInStage = 0;
    deaths = 0;
    bombsUsed = 0;
    reviveProgressFrames = 0;
    team = selectedTeam;
    presence = MULTI_PLAYER_PHYSICAL;
    input.Reset();
}

MultiPlayerState::MultiPlayerState()
{
    Reset(false, 0, 0, 0, 0, 0);
}

void MultiPlayerState::Reset(
    bool shouldEnable,
    u32 p1Team,
    u32 p2Team,
    i32 initialLives,
    i32 initialBombs,
    i32 initialPower)
{
    enabled = shouldEnable;
    frameNumber = 0;
    slots[MULTI_PLAYER_P1].Reset(p1Team, initialLives, initialBombs, initialPower);
    slots[MULTI_PLAYER_P2].Reset(p2Team, initialLives, initialBombs, initialPower);
    if (!enabled)
        slots[MULTI_PLAYER_P2].presence = MULTI_PLAYER_INACTIVE;
}

bool MultiPlayerState::IsEnabled() const
{
    return enabled;
}

MultiPlayerSlotState &MultiPlayerState::GetSlot(MultiPlayerSlot slot)
{
    return slots[slot];
}

const MultiPlayerSlotState &MultiPlayerState::GetSlot(MultiPlayerSlot slot) const
{
    return slots[slot];
}

MultiPlayerInputFrame &MultiPlayerState::GetInput(MultiPlayerSlot slot)
{
    return slots[slot].input;
}

const MultiPlayerInputFrame &MultiPlayerState::GetInput(MultiPlayerSlot slot) const
{
    return slots[slot].input;
}

bool MultiPlayerState::IsPhysical(MultiPlayerSlot slot) const
{
    return slots[slot].presence == MULTI_PLAYER_PHYSICAL;
}

bool MultiPlayerState::IsSpirit(MultiPlayerSlot slot) const
{
    return slots[slot].presence == MULTI_PLAYER_SPIRIT;
}

bool MultiPlayerState::BothPlayersUnableToContinue() const
{
    return enabled && !IsPhysical(MULTI_PLAYER_P1) && !IsPhysical(MULTI_PLAYER_P2);
}

MultiPlayerSlot MultiPlayerState::ResolveNearestPhysicalPlayer(
    const MultiPlayerPosition &target,
    const MultiPlayerPosition positions[MULTI_PLAYER_COUNT]) const
{
    bool p1Eligible = IsPhysical(MULTI_PLAYER_P1);
    bool p2Eligible = enabled && IsPhysical(MULTI_PLAYER_P2);

    if (!p1Eligible)
        return p2Eligible ? MULTI_PLAYER_P2 : MULTI_PLAYER_NONE;
    if (!p2Eligible)
        return MULTI_PLAYER_P1;

    // Strictly-less preserves the documented deterministic P1 tie-break.
    if (DistanceSquared(target, positions[MULTI_PLAYER_P2]) <
        DistanceSquared(target, positions[MULTI_PLAYER_P1]))
        return MULTI_PLAYER_P2;
    return MULTI_PLAYER_P1;
}

void MultiPlayerState::EnterSpirit(MultiPlayerSlot slot)
{
    slots[slot].presence = MULTI_PLAYER_SPIRIT;
    slots[slot].reviveProgressFrames = 0;
}

MultiPlayerReviveResult MultiPlayerState::UpdateRevival(
    MultiPlayerSlot rescuer,
    bool playersOverlap,
    bool focusHeld,
    bool shooting)
{
    MultiPlayerSlot spirit = OtherSlot(rescuer);
    MultiPlayerSlotState &rescuerState = slots[rescuer];
    MultiPlayerSlotState &spiritState = slots[spirit];

    if (!enabled || !IsPhysical(rescuer) || !IsSpirit(spirit))
        return MULTI_REVIVE_NONE;

    // TH06 stores reserve lives: the rescuer must have at least one to give.
    // The revived player's current physical life is restored without adding a
    // reserve-life stock here.
    if (!playersOverlap || !focusHeld || shooting || rescuerState.lives < 1)
    {
        if (rescuerState.reviveProgressFrames != 0)
        {
            rescuerState.reviveProgressFrames = 0;
            return MULTI_REVIVE_CANCELLED;
        }
        return MULTI_REVIVE_NONE;
    }

    ++rescuerState.reviveProgressFrames;
    if (rescuerState.reviveProgressFrames < REVIVE_HOLD_FRAMES)
        return MULTI_REVIVE_PROGRESS;

    rescuerState.reviveProgressFrames = 0;
    --rescuerState.lives;
    spiritState.presence = MULTI_PLAYER_PHYSICAL;
    spiritState.reviveProgressFrames = 0;
    return MULTI_REVIVE_COMPLETED;
}

u32 MultiPlayerState::ComputeStateHash(
    const MultiPlayerPosition positions[MULTI_PLAYER_COUNT],
    const MultiPlayerSharedSnapshot &shared) const
{
    u32 hash = 2166136261u;
    u32 i;

    hash = MixHash(hash, enabled ? 1u : 0u);
    hash = MixHash(hash, frameNumber);
    for (i = 0; i < MULTI_PLAYER_COUNT; ++i)
    {
        const MultiPlayerSlotState &slot = slots[i];
        hash = MixHash(hash, slot.team);
        hash = MixHash(hash, static_cast<u32>(slot.presence));
        hash = MixHash(hash, static_cast<u32>(slot.lives));
        hash = MixHash(hash, static_cast<u32>(slot.bombs));
        hash = MixHash(hash, static_cast<u32>(slot.power));
        hash = MixHash(hash, static_cast<u32>(slot.youkaiGauge));
        hash = MixHash(hash, static_cast<u32>(slot.graze));
        hash = MixHash(hash, static_cast<u32>(slot.grazeInStage));
        hash = MixHash(hash, static_cast<u32>(slot.deaths));
        hash = MixHash(hash, static_cast<u32>(slot.bombsUsed));
        hash = MixHash(hash, static_cast<u32>(slot.reviveProgressFrames));
        hash = MixHash(hash, slot.input.current);
        hash = MixHash(hash, slot.input.previous);
        hash = MixHash(hash, FloatBits(positions[i].x));
        hash = MixHash(hash, FloatBits(positions[i].y));
    }

    hash = MixHash(hash, shared.score);
    hash = MixHash(hash, shared.pointItems);
    hash = MixHash(hash, shared.pointItemValue);
    hash = MixHash(hash, shared.timeOrbs);
    hash = MixHash(hash, shared.totalTimeOrbs);
    hash = MixHash(hash, shared.rngSeed);
    hash = MixHash(hash, shared.activeEnemies);
    hash = MixHash(hash, shared.activeBullets);
    hash = MixHash(hash, static_cast<u32>(shared.bossLife));
    hash = MixHash(hash, static_cast<u32>(shared.bossPhase));
    return hash;
}

} // namespace th08
