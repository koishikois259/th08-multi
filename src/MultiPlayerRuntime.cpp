#include "th_pch.h"
#include "MultiPlayerRuntime.hpp"

#include "GameManager.hpp"
#include "Gui.hpp"
#include "BulletManager.hpp"
#include "EnemyManager.hpp"
#include "Global.hpp"
#include "Player.hpp"

namespace th08
{

MultiPlayerSlot GetMultiPlayerSlot(const Player *player)
{
    return player == &g_Player2 ? MULTI_PLAYER_P2 : MULTI_PLAYER_P1;
}

bool IsMultiPlayerPhysical(const Player *player)
{
    if (!g_MultiPlayerState.IsEnabled())
        return player == &g_Player;
    return g_MultiPlayerState.IsPhysical(GetMultiPlayerSlot(player));
}

Player *GetNearestPhysicalPlayer(const Float3 &target)
{
    MultiPlayerPosition targetPosition;
    MultiPlayerPosition positions[MULTI_PLAYER_COUNT];
    MultiPlayerSlot slot;

    if (!g_MultiPlayerState.IsEnabled())
        return &g_Player;

    targetPosition.x = target.x;
    targetPosition.y = target.y;
    positions[MULTI_PLAYER_P1].x = g_Player.position.x;
    positions[MULTI_PLAYER_P1].y = g_Player.position.y;
    positions[MULTI_PLAYER_P2].x = g_Player2.position.x;
    positions[MULTI_PLAYER_P2].y = g_Player2.position.y;
    slot = g_MultiPlayerState.ResolveNearestPhysicalPlayer(targetPosition, positions);
    if (slot == MULTI_PLAYER_P2)
        return &g_Player2;
    // During the terminal both-spirit frame, retain P1 as a deterministic
    // inert fallback for legacy callers that cannot represent no target.
    return &g_Player;
}

u32 GetMultiPlayerShotType(const Player *player)
{
    if (GetMultiPlayerSlot(player) == MULTI_PLAYER_P2)
        return g_MultiPlayerState.GetSlot(MULTI_PLAYER_P2).team;
    return g_GameManager.shotType;
}

u16 GetMultiPlayerInputCurrent(const Player *player)
{
    if (g_MultiPlayerState.IsEnabled())
        return g_MultiPlayerState.GetInput(GetMultiPlayerSlot(player)).current;
    if (GetMultiPlayerSlot(player) == MULTI_PLAYER_P2)
        return g_MultiPlayerState.GetInput(MULTI_PLAYER_P2).current;
    return g_GuiMessageInputCurrent;
}

u16 GetMultiPlayerInputPrevious(const Player *player)
{
    if (g_MultiPlayerState.IsEnabled())
        return g_MultiPlayerState.GetInput(GetMultiPlayerSlot(player)).previous;
    if (GetMultiPlayerSlot(player) == MULTI_PLAYER_P2)
        return g_MultiPlayerState.GetInput(MULTI_PLAYER_P2).previous;
    return g_GuiMessageInputPrevious;
}

i32 GetMultiPlayerLives(const Player *player)
{
    if (GetMultiPlayerSlot(player) == MULTI_PLAYER_P2)
        return g_MultiPlayerState.GetSlot(MULTI_PLAYER_P2).lives;
    return g_GameManager.GetLives();
}

void SetMultiPlayerLives(Player *player, i32 value)
{
    MultiPlayerSlot slot = GetMultiPlayerSlot(player);
    g_MultiPlayerState.GetSlot(slot).lives = value;
    if (slot == MULTI_PLAYER_P1)
        g_GameManager.SetLives(value);
}

void AddMultiPlayerLives(Player *player, i32 amount)
{
    SetMultiPlayerLives(player, GetMultiPlayerLives(player) + amount);
}

i32 GetMultiPlayerBombs(const Player *player)
{
    if (GetMultiPlayerSlot(player) == MULTI_PLAYER_P2)
        return g_MultiPlayerState.GetSlot(MULTI_PLAYER_P2).bombs;
    return g_GameManager.GetBombsRemaining();
}

void SetMultiPlayerBombs(Player *player, i32 value)
{
    MultiPlayerSlot slot = GetMultiPlayerSlot(player);
    g_MultiPlayerState.GetSlot(slot).bombs = value;
    if (slot == MULTI_PLAYER_P1)
        g_GameManager.SetBombCount(value);
}

void AddMultiPlayerBombs(Player *player, i32 amount)
{
    SetMultiPlayerBombs(player, GetMultiPlayerBombs(player) + amount);
}

i32 GetMultiPlayerPower(const Player *player)
{
    if (GetMultiPlayerSlot(player) == MULTI_PLAYER_P2)
        return g_MultiPlayerState.GetSlot(MULTI_PLAYER_P2).power;
    return g_GameManager.GetPower();
}

void SetMultiPlayerPower(Player *player, i32 value)
{
    MultiPlayerSlot slot = GetMultiPlayerSlot(player);
    g_MultiPlayerState.GetSlot(slot).power = value;
    if (slot == MULTI_PLAYER_P1)
        g_GameManager.SetPower(value);
}

void AddMultiPlayerPower(Player *player, i32 amount)
{
    SetMultiPlayerPower(player, GetMultiPlayerPower(player) + amount);
}

i32 GetMultiPlayerYoukaiGauge(const Player *player)
{
    if (GetMultiPlayerSlot(player) == MULTI_PLAYER_P2)
        return g_MultiPlayerState.GetSlot(MULTI_PLAYER_P2).youkaiGauge;
    return g_GameManager.GetYoukaiGauge();
}

void SetMultiPlayerYoukaiGauge(Player *player, i32 value)
{
    MultiPlayerSlot slot = GetMultiPlayerSlot(player);
    if (value < g_GameManager.youkaiGaugeHumanLimit)
        value = g_GameManager.youkaiGaugeHumanLimit;
    else if (value > g_GameManager.youkaiGaugeYoukaiLimit)
        value = g_GameManager.youkaiGaugeYoukaiLimit;
    g_MultiPlayerState.GetSlot(slot).youkaiGauge = value;
    if (slot == MULTI_PLAYER_P1)
        g_GameManager.SetYoukaiGauge(static_cast<u16>(value));
}

void AddMultiPlayerYoukaiGauge(Player *player, i32 amount, i32 forceUpdate)
{
    if (player->bombState.isInUse && !forceUpdate)
        return;
    if (GetMultiPlayerSlot(player) == MULTI_PLAYER_P1)
    {
        g_GameManager.AddToYoukaiGauge(amount, forceUpdate);
        g_MultiPlayerState.GetSlot(MULTI_PLAYER_P1).youkaiGauge = g_GameManager.GetYoukaiGauge();
    }
    else
    {
        SetMultiPlayerYoukaiGauge(player, GetMultiPlayerYoukaiGauge(player) + amount);
    }
}

i32 GetMultiPlayerGraze(const Player *player)
{
    if (GetMultiPlayerSlot(player) == MULTI_PLAYER_P2)
        return g_MultiPlayerState.GetSlot(MULTI_PLAYER_P2).graze;
    return g_GameManager.globals->graze;
}

i32 GetMultiPlayerGrazeInStage(const Player *player)
{
    if (GetMultiPlayerSlot(player) == MULTI_PLAYER_P2)
        return g_MultiPlayerState.GetSlot(MULTI_PLAYER_P2).grazeInStage;
    return g_GameManager.globals->grazeInStage;
}

void AddMultiPlayerGraze(Player *player, i32 runAmount, i32 stageAmount)
{
    MultiPlayerSlot slot = GetMultiPlayerSlot(player);
    MultiPlayerSlotState &state = g_MultiPlayerState.GetSlot(slot);
    if (state.graze < 999999)
    {
        state.graze += runAmount;
        if (state.graze > 999999)
            state.graze = 999999;
    }
    if (state.grazeInStage < 99999)
    {
        state.grazeInStage += stageAmount;
        if (state.grazeInStage > 99999)
            state.grazeInStage = 99999;
    }
    if (slot == MULTI_PLAYER_P1)
    {
        g_GameManager.globals->graze = state.graze;
        g_GameManager.globals->grazeInStage = state.grazeInStage;
    }
}

void AddMultiPlayerDeath(Player *player)
{
    g_MultiPlayerState.GetSlot(GetMultiPlayerSlot(player)).deaths++;
    g_GameManager.AddToDeaths(1);
}

void AddMultiPlayerBombUsed(Player *player)
{
    g_MultiPlayerState.GetSlot(GetMultiPlayerSlot(player)).bombsUsed++;
    g_GameManager.AddToBombsUsed(1);
}

void SetPhysicalMultiPlayersInvulnerable(i32 frames, i32 bombInputLockFrames)
{
    Player *players[MULTI_PLAYER_COUNT] = {&g_Player, &g_Player2};
    i32 i;
    for (i = 0; i < MULTI_PLAYER_COUNT; ++i)
    {
        if (!g_MultiPlayerState.IsPhysical(static_cast<MultiPlayerSlot>(i)))
            continue;
        players[i]->bombInputLockFrames = bombInputLockFrames;
        if (players[i]->playerState == PLAYER_STATE_ALIVE)
        {
            players[i]->timer = frames;
            players[i]->playerState = PLAYER_STATE_INVULNERABLE;
        }
    }
}

bool MultiPlayerGaugeIsExtremelyHuman(const Player *player)
{
    return GetMultiPlayerYoukaiGauge(player) <= g_GameManager.youkaiGaugeHumanEffectsThreshold;
}

bool MultiPlayerGaugeIsModeratelyHuman(const Player *player)
{
    return GetMultiPlayerYoukaiGauge(player) <= g_GameManager.youkaiGaugeHumanTintThreshold;
}

bool MultiPlayerGaugeIsExtremelyYoukai(const Player *player)
{
    return GetMultiPlayerYoukaiGauge(player) >= g_GameManager.youkaiGaugeYoukaiEffectsThreshold;
}

bool MultiPlayerGaugeIsModeratelyYoukai(const Player *player)
{
    return GetMultiPlayerYoukaiGauge(player) >= g_GameManager.youkaiGaugeYoukaiTintThreshold;
}

bool MultiPlayerShotTypeIsSoloHuman(const Player *player)
{
    u32 shotType = GetMultiPlayerShotType(player);
    return shotType >= 4 && (shotType & 1) == 0;
}

bool MultiPlayerShotTypeIsSoloYoukai(const Player *player)
{
    u32 shotType = GetMultiPlayerShotType(player);
    return shotType >= 4 && (shotType & 1) != 0;
}

bool AllPhysicalMultiPlayersAreYoukai()
{
    Player *players[MULTI_PLAYER_COUNT] = {&g_Player, &g_Player2};
    bool foundPhysical = false;
    i32 i;
    for (i = 0; i < MULTI_PLAYER_COUNT; ++i)
    {
        if (!IsMultiPlayerPhysical(players[i]))
            continue;
        foundPhysical = true;
        if (!players[i]->IsYoukai())
            return false;
    }
    return foundPhysical;
}

bool AnyMultiPlayerBombIsActive()
{
    return (IsMultiPlayerPhysical(&g_Player) && g_Player.bombState.isInUse) ||
           (IsMultiPlayerPhysical(&g_Player2) && g_Player2.bombState.isInUse);
}

bool ShouldPauseEnemyTimerForMultiPlayers()
{
    Player *players[MULTI_PLAYER_COUNT] = {&g_Player, &g_Player2};
    bool foundPhysical = false;
    i32 i;
    if (AnyMultiPlayerBombIsActive())
        return true;
    for (i = 0; i < MULTI_PLAYER_COUNT; ++i)
    {
        if (!IsMultiPlayerPhysical(players[i]))
            continue;
        foundPhysical = true;
        if (players[i]->playerState == PLAYER_STATE_ALIVE ||
            players[i]->playerState == PLAYER_STATE_INVULNERABLE)
            return false;
    }
    return foundPhysical;
}

bool AllMultiPlayersHaveFullPower()
{
    return GetMultiPlayerPower(&g_Player) >= 128 &&
           GetMultiPlayerPower(&g_Player2) >= 128;
}

void SyncP1MultiPlayerResourcesFromGame()
{
    MultiPlayerSlotState &state = g_MultiPlayerState.GetSlot(MULTI_PLAYER_P1);
    state.lives = g_GameManager.GetLives();
    state.bombs = g_GameManager.GetBombsRemaining();
    state.power = g_GameManager.GetPower();
    state.youkaiGauge = g_GameManager.GetYoukaiGauge();
    state.graze = g_GameManager.globals->graze;
    state.grazeInStage = g_GameManager.globals->grazeInStage;
}

static void ResetPlayerObjectAfterContinue(Player *player, MultiPlayerSlot slot)
{
    i32 i;

    if (player->deathbombEffect != NULL)
    {
        player->deathbombEffect->active = 0;
        player->deathbombEffect = NULL;
    }
    if (player->focusEffect != NULL)
    {
        player->focusEffect->active = 0;
        player->focusEffect = NULL;
    }
    if (player->extremeGaugeEffect != NULL)
    {
        player->extremeGaugeEffect->active = 0;
        player->extremeGaugeEffect = NULL;
    }
    if (player->stateEffect != NULL)
    {
        player->stateEffect->active = 0;
        player->stateEffect = NULL;
    }

    player->position.x = g_GameManager.arcadeRegionSize.x / 2.0f;
    player->position.x += slot == MULTI_PLAYER_P1 ? -12.0f : 12.0f;
    player->position.y = g_GameManager.arcadeRegionSize.y - 64.0f;
    player->position.z = 0.2f;
    for (i = 0; i < 16; ++i)
        player->positionHistory[i] = player->position;

    for (i = 0; i < 0x80; ++i)
        player->shots[i].state = PLAYER_SHOT_INACTIVE;

    player->movementDirection = PLAYER_DIRECTION_NONE;
    player->currentHorizontalSpeed = 0.0f;
    player->currentVerticalSpeed = 0.0f;
    player->horizontalSpeedMultiplier = 1.0f;
    player->verticalSpeedMultiplier = 1.0f;
    player->focusMode = PLAYER_FOCUS_MODE_UNINITIALIZED;
    player->focusTransitionFrames = 0;
    player->shotTimer = -1;
    player->gaugeShiftDelayTimer = 0;
    player->shootingGaugeChangeRampTimer = 0;
    player->bombState.isInUse = 0;
    player->bombState.timer = 0;
    player->deathbombWindowFrames = 0;
    player->deathbombPending = 0;
    player->forceDeathbombAtWindowEnd = 0;
    player->bombInputLockFrames = 0;
    player->playerStateSlotCooldown = 0;

    player->playerState = PLAYER_STATE_SPAWNING;
    player->timer = 0;
    player->mainVm.scale.x = 3.0f;
    player->mainVm.scale.y = 3.0f;
    player->mainVm.color1.d3dColor = 0xFFFFFFFF;
    player->mainVm.flagsWord &= ~0x20000u;
}

void ResetMultiPlayersAfterContinue(i32 initialLives, i32 initialPower)
{
    i32 p1InitialBombs = g_GameManager.GetBombsRemaining();
    i32 p2InitialBombs = p1InitialBombs;

    if (!g_MultiPlayerState.IsEnabled())
        return;

    if (g_Player.primaryShtFile != NULL)
        p1InitialBombs = static_cast<i32>(g_Player.primaryShtFile->initialBombCount);
    if (g_Player2.primaryShtFile != NULL)
        p2InitialBombs = static_cast<i32>(g_Player2.primaryShtFile->initialBombCount);

    g_MultiPlayerState.ResetForContinue(
        initialLives, p1InitialBombs, p2InitialBombs, initialPower);

    g_GameManager.SetLives(initialLives);
    g_GameManager.SetBombCount(p1InitialBombs);
    g_GameManager.SetPower(initialPower);
    g_GameManager.SetYoukaiGauge(0);

    ResetPlayerObjectAfterContinue(&g_Player, MULTI_PLAYER_P1);
    ResetPlayerObjectAfterContinue(&g_Player2, MULTI_PLAYER_P2);
    SyncP1MultiPlayerResourcesFromGame();

    g_Gui.flags.lifeDisplayUpdateFrames = 2;
    g_Gui.flags.bombDisplayUpdateFrames = 2;
    g_Gui.flags.powerDisplayUpdateFrames = 2;
}

void RestoreMultiPlayerPresenceForStage()
{
    Player *players[MULTI_PLAYER_COUNT] = {&g_Player, &g_Player2};
    i32 i;

    if (!g_MultiPlayerState.IsEnabled())
        return;

    for (i = 0; i < MULTI_PLAYER_COUNT; ++i)
    {
        Player *player = players[i];
        if (!g_MultiPlayerState.IsSpirit(static_cast<MultiPlayerSlot>(i)))
            continue;

        // Registering a Player for the next stage initializes it as spawning.
        // Reapply the persistent co-op presence before the first gameplay tick
        // so a player with no remaining life cannot silently return.
        player->playerState = PLAYER_STATE_SPIRIT;
        player->timer = 0;
        player->bombState.isInUse = 0;
        player->deathbombWindowFrames = 0;
        player->mainVm.color1.a = 80;
    }
}

void UpdateMultiPlayerSpiritMotion(Player *player)
{
    MultiPlayerPosition position;
    f32 oldX;
    f32 oldY;
    f32 deltaX;
    f32 deltaY;
    i32 i;

    if (!g_MultiPlayerState.IsEnabled() ||
        !g_MultiPlayerState.IsSpirit(GetMultiPlayerSlot(player)))
        return;

    oldX = player->position.x;
    oldY = player->position.y;
    position.x = oldX;
    position.y = oldY;
    g_MultiPlayerState.UpdateSpiritPosition(
        GetMultiPlayerSlot(player),
        &position,
        g_GameManager.playerMovementTopLeftPos.x,
        g_GameManager.playerMovementTopLeftPos.y,
        g_GameManager.playerMovementTopLeftPos.x +
            g_GameManager.playerMovementAreaSize.x,
        g_GameManager.playerMovementTopLeftPos.y +
            g_GameManager.playerMovementAreaSize.y);
    player->position.x = position.x;
    player->position.y = position.y;

    // Options do not run their normal formation update while their owner is a
    // spirit. Move the frozen formation by the same amount so it cannot be
    // left behind at the death position.
    deltaX = position.x - oldX;
    deltaY = position.y - oldY;
    for (i = 0; i < 4; ++i)
    {
        player->optionStates[i].position.x += deltaX;
        player->optionStates[i].position.y += deltaY;
    }

}

u32 ComputeCurrentMultiPlayerStateHash()
{
    MultiPlayerPosition positions[MULTI_PLAYER_COUNT];
    MultiPlayerSharedSnapshot shared;
    Enemy *boss = NULL;
    i32 i;

    SyncP1MultiPlayerResourcesFromGame();
    positions[MULTI_PLAYER_P1].x = g_Player.position.x;
    positions[MULTI_PLAYER_P1].y = g_Player.position.y;
    positions[MULTI_PLAYER_P2].x = g_Player2.position.x;
    positions[MULTI_PLAYER_P2].y = g_Player2.position.y;
    shared.score = g_GameManager.globals->score;
    shared.pointItems = static_cast<u32>(g_GameManager.globals->pointItemsCollected);
    shared.pointItemValue = static_cast<u32>(g_GameManager.globals->pointItemValue);
    shared.timeOrbs = static_cast<u32>(g_GameManager.globals->currentTimeOrbs);
    shared.totalTimeOrbs = static_cast<u32>(g_GameManager.globals->totalTimeOrbs);
    shared.rngSeed = g_Rng.GetSeed();
    shared.activeEnemies = static_cast<u32>(g_EnemyManager.activeEnemyCount);
    shared.activeBullets = static_cast<u32>(g_BulletManager.activeBulletCount);
    for (i = 0; i < 8; ++i)
    {
        if (g_EnemyManager.bosses[i] != NULL)
        {
            boss = g_EnemyManager.bosses[i];
            break;
        }
    }
    shared.bossLife = boss != NULL ? boss->life : 0;
    shared.bossPhase = g_GameManager.currentSpellCardNumber;
    return g_MultiPlayerState.ComputeStateHash(positions, shared);
}

MultiPlayerReviveResult UpdateMultiPlayerRevival(Player *rescuer)
{
    MultiPlayerSlot rescuerSlot;
    Player *spirit;
    f32 x;
    f32 y;
    bool overlap;
    u16 input;
    MultiPlayerReviveResult result;

    if (!g_MultiPlayerState.IsEnabled())
        return MULTI_REVIVE_NONE;

    rescuerSlot = GetMultiPlayerSlot(rescuer);
    spirit = rescuerSlot == MULTI_PLAYER_P1 ? &g_Player2 : &g_Player;
    x = rescuer->position.x - spirit->position.x;
    y = rescuer->position.y - spirit->position.y;
    overlap = x * x + y * y <= 24.0f * 24.0f;
    input = GetMultiPlayerInputCurrent(rescuer);
    result = g_MultiPlayerState.UpdateRevival(
        rescuerSlot,
        overlap,
        (input & TH_BUTTON_FOCUS) != 0,
        (input & TH_BUTTON_SHOOT) != 0);

    if (result == MULTI_REVIVE_COMPLETED)
    {
        if (rescuerSlot == MULTI_PLAYER_P1)
            g_GameManager.SetLives(g_MultiPlayerState.GetSlot(MULTI_PLAYER_P1).lives);
        spirit->playerState = PLAYER_STATE_SPAWNING;
        spirit->timer = 0;
        spirit->mainVm.scale.x = 3.0f;
        spirit->mainVm.scale.y = 3.0f;
        spirit->mainVm.color1.d3dColor = 0xFFFFFFFF;
    }
    return result;
}

} // namespace th08
