#include "th_pch.h"
#include "MultiPlayerRuntime.hpp"

#include "GameManager.hpp"
#include "Gui.hpp"
#include "Player.hpp"

namespace th08
{

MultiPlayerSlot GetMultiPlayerSlot(const Player *player)
{
    return player == &g_Player2 ? MULTI_PLAYER_P2 : MULTI_PLAYER_P1;
}

u32 GetMultiPlayerShotType(const Player *player)
{
    if (GetMultiPlayerSlot(player) == MULTI_PLAYER_P2)
        return g_MultiPlayerState.GetSlot(MULTI_PLAYER_P2).team;
    return g_GameManager.shotType;
}

u16 GetMultiPlayerInputCurrent(const Player *player)
{
    if (GetMultiPlayerSlot(player) == MULTI_PLAYER_P2)
        return g_MultiPlayerState.GetInput(MULTI_PLAYER_P2).current;
    return g_GuiMessageInputCurrent;
}

u16 GetMultiPlayerInputPrevious(const Player *player)
{
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
