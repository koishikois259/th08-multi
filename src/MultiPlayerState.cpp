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

u32 NextSpiritRandom(u32 value)
{
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    return value;
}

// Axis-aligned directions are intentionally excluded. Every launch keeps at
// least a 22.5-degree angle from a playfield-edge normal, so a spirit cannot
// start by travelling perpendicularly into any edge.
const MultiPlayerPosition g_SpiritDirections[12] = {
    {0.9238795f, 0.3826834f}, {0.7071068f, 0.7071068f},
    {0.3826834f, 0.9238795f}, {-0.3826834f, 0.9238795f},
    {-0.7071068f, 0.7071068f}, {-0.9238795f, 0.3826834f},
    {-0.9238795f, -0.3826834f}, {-0.7071068f, -0.7071068f},
    {-0.3826834f, -0.9238795f}, {0.3826834f, -0.9238795f},
    {0.7071068f, -0.7071068f}, {0.9238795f, -0.3826834f},
};

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
    spiritVelocity.x = 0.0f;
    spiritVelocity.y = 0.0f;
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

void MultiPlayerState::ResetForContinue(
    i32 initialLives,
    i32 p1InitialBombs,
    i32 p2InitialBombs,
    i32 initialPower)
{
    u32 p1Team = slots[MULTI_PLAYER_P1].team;
    u32 p2Team = slots[MULTI_PLAYER_P2].team;
    u32 continueFrameNumber = frameNumber;

    Reset(true, p1Team, p2Team, initialLives, p1InitialBombs, initialPower);
    frameNumber = continueFrameNumber;
    slots[MULTI_PLAYER_P2].bombs = p2InitialBombs;
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

bool MultiPlayerState::CanReceiveSharedLifeExtend(
    MultiPlayerSlot slot, i32 maximumLives) const
{
    return IsPhysical(slot) && slots[slot].lives < maximumLives;
}

bool MultiPlayerState::BothPlayersUnableToContinue() const
{
    return enabled && !IsPhysical(MULTI_PLAYER_P1) && !IsPhysical(MULTI_PLAYER_P2);
}

void MultiPlayerState::ResetStageCounters()
{
    slots[MULTI_PLAYER_P1].grazeInStage = 0;
    slots[MULTI_PLAYER_P2].grazeInStage = 0;
}

i32 MultiPlayerState::GetCombinedGrazeInStage() const
{
    return slots[MULTI_PLAYER_P1].grazeInStage +
           slots[MULTI_PLAYER_P2].grazeInStage;
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
    u32 randomValue;
    f32 speed;

    slots[slot].presence = MULTI_PLAYER_SPIRIT;
    slots[slot].reviveProgressFrames = 0;
    randomValue = frameNumber ^
                  (0x9e3779b9u * (static_cast<u32>(slot) + 1u)) ^
                  (0x85ebca6bu * (static_cast<u32>(slots[slot].deaths) + 1u));
    randomValue = NextSpiritRandom(randomValue);
    speed = 0.66f +
            static_cast<f32>((randomValue >> 8) & 0xffu) *
                (0.48f / 255.0f);
    slots[slot].spiritVelocity.x =
        g_SpiritDirections[randomValue % 12u].x * speed;
    slots[slot].spiritVelocity.y =
        g_SpiritDirections[randomValue % 12u].y * speed;
}

void MultiPlayerState::UpdateSpiritPosition(
    MultiPlayerSlot slot,
    MultiPlayerPosition *position,
    f32 left,
    f32 top,
    f32 right,
    f32 bottom)
{
    MultiPlayerPosition &velocity = slots[slot].spiritVelocity;

    if (!enabled || !IsSpirit(slot) || position == NULL)
        return;

    position->x += velocity.x;
    position->y += velocity.y;

    if (position->x < left)
    {
        position->x = left + (left - position->x);
        velocity.x = -velocity.x;
    }
    else if (position->x > right)
    {
        position->x = right - (position->x - right);
        velocity.x = -velocity.x;
    }

    if (position->y < top)
    {
        position->y = top + (top - position->y);
        velocity.y = -velocity.y;
    }
    else if (position->y > bottom)
    {
        position->y = bottom - (position->y - bottom);
        velocity.y = -velocity.y;
    }
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
    spiritState.power = 128;
    spiritState.reviveProgressFrames = 0;
    spiritState.spiritVelocity.x = 0.0f;
    spiritState.spiritVelocity.y = 0.0f;
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
        hash = MixHash(hash, FloatBits(slot.spiritVelocity.x));
        hash = MixHash(hash, FloatBits(slot.spiritVelocity.y));
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
    hash = MixHash(hash, shared.currentStage);
    hash = MixHash(hash, shared.finalStageRoute);
    hash = MixHash(hash, shared.stageTransitionState);
    hash = MixHash(hash, static_cast<u32>(shared.dialogueMessage));
    hash = MixHash(hash, shared.dialogueOption);
    return hash;
}

} // namespace th08
