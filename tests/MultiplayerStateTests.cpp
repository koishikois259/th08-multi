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
    Expect(state.GetSlot(MULTI_PLAYER_P2).power == 128,
           "revived spirit returns at full power");
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

void TestRevivalRequiresEveryInteractionCondition()
{
    MultiPlayerState state;
    state.Reset(true, 0, 1, 2, 3, 0);
    state.EnterSpirit(MULTI_PLAYER_P2);

    Expect(
        state.UpdateRevival(MULTI_PLAYER_P1, false, true, false) == MULTI_REVIVE_NONE,
        "revival requires overlap");
    Expect(
        state.UpdateRevival(MULTI_PLAYER_P1, true, false, false) == MULTI_REVIVE_NONE,
        "revival requires Focus");
    Expect(
        state.UpdateRevival(MULTI_PLAYER_P1, true, true, true) == MULTI_REVIVE_NONE,
        "revival requires no shooting");

    Expect(
        state.UpdateRevival(MULTI_PLAYER_P1, true, true, false) == MULTI_REVIVE_PROGRESS,
        "valid revival starts progress");
    Expect(
        state.UpdateRevival(MULTI_PLAYER_P1, false, true, false) == MULTI_REVIVE_CANCELLED,
        "breaking overlap cancels active progress");
}

void TestGameOverRequiresBothPlayersUnable()
{
    MultiPlayerState state;
    state.Reset(true, 0, 1, 0, 3, 0);

    Expect(!state.BothPlayersUnableToContinue(), "two physical players can continue");
    state.EnterSpirit(MULTI_PLAYER_P1);
    Expect(!state.BothPlayersUnableToContinue(), "one physical player keeps the run alive");
    state.EnterSpirit(MULTI_PLAYER_P2);
    Expect(state.BothPlayersUnableToContinue(), "two spirit players end the run");
}

void TestSharedLifeExtendOnlyTargetsPhysicalPlayers()
{
    MultiPlayerState state;
    state.Reset(true, 0, 1, 2, 3, 0);

    Expect(state.CanReceiveSharedLifeExtend(MULTI_PLAYER_P1, 8),
           "physical P1 can receive a shared point extend");
    Expect(state.CanReceiveSharedLifeExtend(MULTI_PLAYER_P2, 8),
           "physical P2 can receive a shared point extend");

    state.EnterSpirit(MULTI_PLAYER_P1);
    state.GetSlot(MULTI_PLAYER_P1).lives = 0;
    Expect(!state.CanReceiveSharedLifeExtend(MULTI_PLAYER_P1, 8),
           "spirit player cannot receive a shared point extend");
    Expect(state.CanReceiveSharedLifeExtend(MULTI_PLAYER_P2, 8),
           "surviving player still receives a shared point extend");

    state.GetSlot(MULTI_PLAYER_P2).lives = 8;
    Expect(!state.CanReceiveSharedLifeExtend(MULTI_PLAYER_P2, 8),
           "life-capped physical player cannot receive another extend");
}

void TestContinueRestoresBothPlayersWithFullPower()
{
    MultiPlayerState state;
    state.Reset(true, 2, 7, 0, 1, 0);
    state.EnterSpirit(MULTI_PLAYER_P1);
    state.EnterSpirit(MULTI_PLAYER_P2);
    state.GetSlot(MULTI_PLAYER_P1).youkaiGauge = -8000;
    state.GetSlot(MULTI_PLAYER_P2).youkaiGauge = 8000;
    state.GetSlot(MULTI_PLAYER_P1).reviveProgressFrames = 45;
    state.GetSlot(MULTI_PLAYER_P2).reviveProgressFrames = 60;
    state.frameNumber = 4242;

    state.ResetForContinue(2, 3, 2, 128);

    Expect(state.IsPhysical(MULTI_PLAYER_P1), "continue restores P1 physical presence");
    Expect(state.IsPhysical(MULTI_PLAYER_P2), "continue restores P2 physical presence");
    Expect(state.GetSlot(MULTI_PLAYER_P1).team == 2, "continue retains P1 team");
    Expect(state.GetSlot(MULTI_PLAYER_P2).team == 7, "continue retains P2 team");
    Expect(state.GetSlot(MULTI_PLAYER_P1).lives == 2 &&
               state.GetSlot(MULTI_PLAYER_P2).lives == 2,
           "continue restores both life stocks");
    Expect(state.GetSlot(MULTI_PLAYER_P1).bombs == 3 &&
               state.GetSlot(MULTI_PLAYER_P2).bombs == 2,
           "continue restores each team's initial bombs");
    Expect(state.GetSlot(MULTI_PLAYER_P1).power == 128 &&
               state.GetSlot(MULTI_PLAYER_P2).power == 128,
           "continue starts both players at 128 power");
    Expect(state.GetSlot(MULTI_PLAYER_P1).youkaiGauge == 0 &&
               state.GetSlot(MULTI_PLAYER_P2).youkaiGauge == 0,
           "continue resets both human-youkai gauges");
    Expect(state.GetSlot(MULTI_PLAYER_P1).reviveProgressFrames == 0 &&
               state.GetSlot(MULTI_PLAYER_P2).reviveProgressFrames == 0,
           "continue clears revival progress");
    Expect(state.frameNumber == 4242,
           "continue preserves the active network simulation frame");
}

void TestCombinedStageGrazeAndReset()
{
    MultiPlayerState state;
    state.Reset(true, 0, 1, 2, 3, 0);
    state.GetSlot(MULTI_PLAYER_P1).graze = 1000;
    state.GetSlot(MULTI_PLAYER_P2).graze = 2000;
    state.GetSlot(MULTI_PLAYER_P1).grazeInStage = 123;
    state.GetSlot(MULTI_PLAYER_P2).grazeInStage = 456;

    Expect(state.GetCombinedGrazeInStage() == 579,
           "stage-clear graze combines P1 and P2");
    state.ResetStageCounters();
    Expect(state.GetSlot(MULTI_PLAYER_P1).grazeInStage == 0,
           "new stage clears P1 stage graze");
    Expect(state.GetSlot(MULTI_PLAYER_P2).grazeInStage == 0,
           "new stage clears P2 stage graze");
    Expect(state.GetSlot(MULTI_PLAYER_P1).graze == 1000,
           "new stage retains P1 run graze");
    Expect(state.GetSlot(MULTI_PLAYER_P2).graze == 2000,
           "new stage retains P2 run graze");
}

void TestSpiritDriftIsDeterministicAndBounces()
{
    MultiPlayerState first;
    MultiPlayerState second;
    MultiPlayerPosition firstPosition = { 100.0f, 200.0f };
    MultiPlayerPosition secondPosition = firstPosition;
    MultiPlayerPosition edgePosition = { 375.8f, 200.0f };
    f32 speedSquared;
    i32 frame;

    first.Reset(true, 0, 1, 2, 3, 0);
    second.Reset(true, 0, 1, 2, 3, 0);
    first.frameNumber = second.frameNumber = 1234;
    first.EnterSpirit(MULTI_PLAYER_P1);
    second.EnterSpirit(MULTI_PLAYER_P1);

    Expect(first.GetSlot(MULTI_PLAYER_P1).spiritVelocity.x ==
               second.GetSlot(MULTI_PLAYER_P1).spiritVelocity.x &&
           first.GetSlot(MULTI_PLAYER_P1).spiritVelocity.y ==
               second.GetSlot(MULTI_PLAYER_P1).spiritVelocity.y,
           "spirit launch velocity is deterministic");
    speedSquared =
        first.GetSlot(MULTI_PLAYER_P1).spiritVelocity.x *
            first.GetSlot(MULTI_PLAYER_P1).spiritVelocity.x +
        first.GetSlot(MULTI_PLAYER_P1).spiritVelocity.y *
            first.GetSlot(MULTI_PLAYER_P1).spiritVelocity.y;
    Expect(speedSquared >= 0.66f * 0.66f &&
               speedSquared <= 1.14f * 1.14f + 0.0001f,
           "spirit launch speed stays in the slow range");

    for (frame = 0; frame < 256; ++frame)
    {
        first.frameNumber = static_cast<u32>(frame);
        first.EnterSpirit(MULTI_PLAYER_P1);
        Expect(first.GetSlot(MULTI_PLAYER_P1).spiritVelocity.x != 0.0f &&
                   first.GetSlot(MULTI_PLAYER_P1).spiritVelocity.y != 0.0f,
               "spirit launch direction is never perpendicular to an edge");
    }
    first.frameNumber = 1234;
    first.EnterSpirit(MULTI_PLAYER_P1);

    first.UpdateSpiritPosition(
        MULTI_PLAYER_P1, &firstPosition, 8.0f, 16.0f, 376.0f, 432.0f);
    second.UpdateSpiritPosition(
        MULTI_PLAYER_P1, &secondPosition, 8.0f, 16.0f, 376.0f, 432.0f);
    Expect(firstPosition.x == secondPosition.x &&
               firstPosition.y == secondPosition.y,
           "spirit movement remains deterministic");

    first.GetSlot(MULTI_PLAYER_P1).spiritVelocity.x = 0.75f;
    first.GetSlot(MULTI_PLAYER_P1).spiritVelocity.y = 0.0f;
    first.UpdateSpiritPosition(
        MULTI_PLAYER_P1, &edgePosition, 8.0f, 16.0f, 376.0f, 432.0f);
    Expect(edgePosition.x <= 376.0f,
           "spirit remains inside the playfield after collision");
    Expect(first.GetSlot(MULTI_PLAYER_P1).spiritVelocity.x < 0.0f,
           "spirit reflects from the playfield edge");
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

void TestEverySharedHudResourceAffectsStateHash()
{
    MultiPlayerState state;
    MultiPlayerPosition positions[MULTI_PLAYER_COUNT] = {
        { 100.0f, 200.0f },
        { 200.0f, 200.0f },
    };
    MultiPlayerSharedSnapshot shared = EmptySharedSnapshot();
    u32 original;

    state.Reset(true, 0, 1, 2, 3, 0);
    original = state.ComputeStateHash(positions, shared);
    ++shared.score;
    Expect(original != state.ComputeStateHash(positions, shared), "shared score changes the hash");
    --shared.score;
    ++shared.pointItems;
    Expect(original != state.ComputeStateHash(positions, shared), "shared point count changes the hash");
    --shared.pointItems;
    ++shared.pointItemValue;
    Expect(original != state.ComputeStateHash(positions, shared), "shared maximum point value changes the hash");
    --shared.pointItemValue;
    ++shared.totalTimeOrbs;
    Expect(original != state.ComputeStateHash(positions, shared), "shared total Time changes the hash");
}

} // namespace

int main()
{
    TestIndependentSlotsAndDuplicateTeams();
    TestInputEdges();
    TestNearestPhysicalPlayer();
    TestSpiritRevival();
    TestNoRevivalWithoutReserveLife();
    TestRevivalRequiresEveryInteractionCondition();
    TestGameOverRequiresBothPlayersUnable();
    TestSharedLifeExtendOnlyTargetsPhysicalPlayers();
    TestContinueRestoresBothPlayersWithFullPower();
    TestCombinedStageGrazeAndReset();
    TestSpiritDriftIsDeterministicAndBounces();
    TestStateHash();
    TestEverySharedHudResourceAffectsStateHash();

    if (failures != 0)
    {
        printf("%d multiplayer state test(s) failed\n", failures);
        return 1;
    }
    printf("multiplayer state tests passed\n");
    return 0;
}
