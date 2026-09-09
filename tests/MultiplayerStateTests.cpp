#include "MultiPlayerState.hpp"
#include <stdio.h>

using namespace th08;

namespace
{

int failures = 0;

void Expect(bool condition, const char *message)
{
    if (!condition)
    {
        ++failures;
        printf("FAIL: %s\n", message);
    }
}

MultiPlayerSharedSnapshot EmptySharedSnapshot()
{
    MultiPlayerSharedSnapshot result = { 0 };
    return result;
}

void TestIndependentSlotsAndDuplicateTeams()
{
    MultiPlayerState state;
    state.Reset(true, 3, 3, 2, 3, 0);

    Expect(state.GetSlot(MULTI_PLAYER_P1).team == 3, "P1 team is retained");
    Expect(state.GetSlot(MULTI_PLAYER_P2).team == 3, "duplicate P2 team is allowed");
    state.GetSlot(MULTI_PLAYER_P1).bombs = 1;
    state.GetSlot(MULTI_PLAYER_P2).power = 64;
    state.GetSlot(MULTI_PLAYER_P1).youkaiGauge = -8000;
    state.GetSlot(MULTI_PLAYER_P2).youkaiGauge = 8000;
    state.GetSlot(MULTI_PLAYER_P1).graze = 10;
    state.GetSlot(MULTI_PLAYER_P2).graze = 20;

    Expect(state.GetSlot(MULTI_PLAYER_P2).bombs == 3, "P1 bomb use cannot change P2 bombs");
    Expect(state.GetSlot(MULTI_PLAYER_P1).power == 0, "P2 power cannot change P1 power");
    Expect(state.GetSlot(MULTI_PLAYER_P1).youkaiGauge == -8000, "P1 gauge is independent");
    Expect(state.GetSlot(MULTI_PLAYER_P2).youkaiGauge == 8000, "P2 gauge is independent");
    Expect(state.GetSlot(MULTI_PLAYER_P1).graze == 10, "P1 graze is independent");
    Expect(state.GetSlot(MULTI_PLAYER_P2).graze == 20, "P2 graze is independent");
}

void TestInputEdges()
{
    MultiPlayerInputFrame input;
    input.Reset();
    input.Advance(1);
    Expect(input.IsPressed(1), "current input is pressed");
    Expect(input.WasPressed(1), "first input frame has an edge");
    input.Advance(1);
    Expect(!input.WasPressed(1), "held input does not repeat an edge");
    input.Advance(0);
    Expect(!input.IsPressed(1), "released input is not pressed");
}

void TestNearestPhysicalPlayer()
{
    MultiPlayerState state;
    MultiPlayerPosition target = { 10.0f, 10.0f };
    MultiPlayerPosition positions[MULTI_PLAYER_COUNT] = {
        { 0.0f, 10.0f },
        { 20.0f, 10.0f },
    };
    state.Reset(true, 0, 1, 2, 3, 0);

    Expect(
        state.ResolveNearestPhysicalPlayer(target, positions) == MULTI_PLAYER_P1,
        "equal distance resolves to P1");
    positions[MULTI_PLAYER_P2].x = 11.0f;
    Expect(
        state.ResolveNearestPhysicalPlayer(target, positions) == MULTI_PLAYER_P2,
        "closer P2 is selected");
    state.EnterSpirit(MULTI_PLAYER_P2);
    Expect(
        state.ResolveNearestPhysicalPlayer(target, positions) == MULTI_PLAYER_P1,
        "spirit player is not a target or collector");
    state.EnterSpirit(MULTI_PLAYER_P1);
    Expect(
        state.ResolveNearestPhysicalPlayer(target, positions) == MULTI_PLAYER_NONE,
        "no physical player produces no target");
}

void TestSpiritRevival()
{
    MultiPlayerState state;
    int frame;
    state.Reset(true, 0, 1, 2, 3, 0);
    state.EnterSpirit(MULTI_PLAYER_P2);

    for (frame = 0; frame < 20; ++frame)
        Expect(
            state.UpdateRevival(MULTI_PLAYER_P1, true, true, false) == MULTI_REVIVE_PROGRESS,
            "valid hold advances revival");
    Expect(
        state.UpdateRevival(MULTI_PLAYER_P1, true, true, true) == MULTI_REVIVE_CANCELLED,
        "shooting cancels revival");
    Expect(state.GetSlot(MULTI_PLAYER_P1).reviveProgressFrames == 0, "cancel resets revival progress");

    for (frame = 1; frame < MultiPlayerState::REVIVE_HOLD_FRAMES; ++frame)
        Expect(
            state.UpdateRevival(MULTI_PLAYER_P1, true, true, false) == MULTI_REVIVE_PROGRESS,
            "revival remains pending before frame 90");
    Expect(
        state.UpdateRevival(MULTI_PLAYER_P1, true, true, false) == MULTI_REVIVE_COMPLETED,
        "frame 90 completes revival");
    Expect(state.IsPhysical(MULTI_PLAYER_P2), "spirit becomes physical");
    Expect(state.GetSlot(MULTI_PLAYER_P1).lives == 1, "rescuer spends one reserve life");
}

void TestNoRevivalWithoutReserveLife()
{
    MultiPlayerState state;
    state.Reset(true, 0, 1, 0, 3, 0);
    state.EnterSpirit(MULTI_PLAYER_P2);
    Expect(
        state.UpdateRevival(MULTI_PLAYER_P1, true, true, false) == MULTI_REVIVE_NONE,
        "zero reserve lives cannot revive");
    Expect(state.IsSpirit(MULTI_PLAYER_P2), "failed revival keeps spirit state");
}

void TestStateHash()
{
    MultiPlayerState state;
    MultiPlayerPosition positions[MULTI_PLAYER_COUNT] = {
        { 100.0f, 200.0f },
        { 200.0f, 200.0f },
    };
    MultiPlayerSharedSnapshot shared = EmptySharedSnapshot();
    u32 original;

    state.Reset(true, 0, 1, 2, 3, 0);
    shared.score = 100;
    shared.timeOrbs = 25;
    shared.rngSeed = 1234;
    original = state.ComputeStateHash(positions, shared);
    Expect(original == state.ComputeStateHash(positions, shared), "identical state hashes identically");
    ++shared.timeOrbs;
    Expect(original != state.ComputeStateHash(positions, shared), "shared Time changes the hash");
    --shared.timeOrbs;
    ++state.GetSlot(MULTI_PLAYER_P2).graze;
    Expect(original != state.ComputeStateHash(positions, shared), "P2 state changes the hash");
}

} // namespace

int main()
{
    TestIndependentSlotsAndDuplicateTeams();
    TestInputEdges();
    TestNearestPhysicalPlayer();
    TestSpiritRevival();
    TestNoRevivalWithoutReserveLife();
    TestStateHash();

    if (failures != 0)
    {
        printf("%d multiplayer state test(s) failed\n", failures);
        return 1;
    }
    printf("multiplayer state tests passed\n");
    return 0;
}
