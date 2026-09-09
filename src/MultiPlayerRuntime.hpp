#pragma once

#include "MultiPlayerState.hpp"

namespace th08
{

struct Player;
struct Float3;

MultiPlayerSlot GetMultiPlayerSlot(const Player *player);
bool IsMultiPlayerPhysical(const Player *player);
Player *GetNearestPhysicalPlayer(const Float3 &target);
u32 GetMultiPlayerShotType(const Player *player);
u16 GetMultiPlayerInputCurrent(const Player *player);
u16 GetMultiPlayerInputPrevious(const Player *player);

i32 GetMultiPlayerLives(const Player *player);
void SetMultiPlayerLives(Player *player, i32 value);
void AddMultiPlayerLives(Player *player, i32 amount);
i32 GetMultiPlayerBombs(const Player *player);
void SetMultiPlayerBombs(Player *player, i32 value);
void AddMultiPlayerBombs(Player *player, i32 amount);
i32 GetMultiPlayerPower(const Player *player);
void SetMultiPlayerPower(Player *player, i32 value);
void AddMultiPlayerPower(Player *player, i32 amount);
i32 GetMultiPlayerYoukaiGauge(const Player *player);
void SetMultiPlayerYoukaiGauge(Player *player, i32 value);
void AddMultiPlayerYoukaiGauge(Player *player, i32 amount, i32 forceUpdate);
i32 GetMultiPlayerGraze(const Player *player);
i32 GetMultiPlayerGrazeInStage(const Player *player);
void AddMultiPlayerGraze(Player *player, i32 runAmount, i32 stageAmount);
void AddMultiPlayerDeath(Player *player);
void AddMultiPlayerBombUsed(Player *player);

bool MultiPlayerGaugeIsExtremelyHuman(const Player *player);
bool MultiPlayerGaugeIsModeratelyHuman(const Player *player);
bool MultiPlayerGaugeIsExtremelyYoukai(const Player *player);
bool MultiPlayerGaugeIsModeratelyYoukai(const Player *player);
bool MultiPlayerShotTypeIsSoloHuman(const Player *player);
bool MultiPlayerShotTypeIsSoloYoukai(const Player *player);

void SyncP1MultiPlayerResourcesFromGame();
MultiPlayerReviveResult UpdateMultiPlayerRevival(Player *rescuer);

} // namespace th08
