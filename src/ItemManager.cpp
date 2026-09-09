#include "th_pch.h"

#include "BulletManager.hpp"
#include "EclManager.hpp"
#include "GameManager.hpp"
#include "Gui.hpp"
#include "ItemManager.hpp"
#include "Player.hpp"
#ifdef TH08_MULTI
#include "MultiPlayerRuntime.hpp"
#endif
#include "ReplayManager.hpp"
#include "Spellcard.hpp"

namespace th08
{

DIFFABLE_STATIC(ItemManager, g_ItemManager);
DIFFABLE_STATIC(i32, g_MaxValuePointItemsCollected);
DIFFABLE_STATIC_ARRAY_ASSIGN(i32, 6, g_PowerUpThresholds) = {8, 24, 48, 80, 128, 999};

#ifdef TH08_MULTI
static bool IsPhysicalItemCollector(Player *player)
{
    return player->primaryShtFile != NULL &&
           g_MultiPlayerState.IsPhysical(GetMultiPlayerSlot(player)) &&
           player->playerState != PLAYER_STATE_DYING &&
           player->playerState != PLAYER_STATE_SPAWNING;
}

static f32 ItemDistanceSquared(const Float3 &itemPosition, const Player *player)
{
    f32 x = itemPosition.x - player->position.x;
    f32 y = itemPosition.y - player->position.y;
    return x * x + y * y;
}

static Player *ResolveNearestItemPlayer(const Float3 &itemPosition,
                                        Player *p1, Player *p2)
{
    if (p1 == NULL)
        return p2;
    if (p2 == NULL)
        return p1;
    if (ItemDistanceSquared(itemPosition, p2) < ItemDistanceSquared(itemPosition, p1))
        return p2;
    return p1;
}

static Player *ResolveNearestPhysicalItemPlayer(const Float3 &itemPosition)
{
    Player *p1 = IsPhysicalItemCollector(&g_Player) ? &g_Player : NULL;
    Player *p2 = IsPhysicalItemCollector(&g_Player2) ? &g_Player2 : NULL;
    return ResolveNearestItemPlayer(itemPosition, p1, p2);
}

static bool IsPointOfCollectionActive(Player *player)
{
    u32 shotType;

    if (!IsPhysicalItemCollector(player))
        return false;
    shotType = GetMultiPlayerShotType(player);
    return player->position.y < player->primaryShtFile->pointItemValueLine &&
           (GetMultiPlayerPower(player) >= 128 ||
            player->focusMode != PLAYER_FOCUS_MODE_UNFOCUSED ||
            shotType == 1 || shotType == 6);
}

static Player *ResolvePointOfCollectionPlayer(const Float3 &itemPosition)
{
    Player *p1 = IsPointOfCollectionActive(&g_Player) ? &g_Player : NULL;
    Player *p2 = IsPointOfCollectionActive(&g_Player2) ? &g_Player2 : NULL;
    return ResolveNearestItemPlayer(itemPosition, p1, p2);
}

static Player *ResolveCollidingItemPlayer(const Float3 &itemPosition)
{
    Player *p1 = NULL;
    Player *p2 = NULL;

    if (IsPhysicalItemCollector(&g_Player))
    {
        Float3 itemBox(g_Player.primaryShtFile->itemCollectionBoxSize,
                       g_Player.primaryShtFile->itemCollectionBoxSize, 16.0f);
        if (g_Player.CalcItemBoxCollision(const_cast<Float3 *>(&itemPosition), &itemBox))
            p1 = &g_Player;
    }
    if (IsPhysicalItemCollector(&g_Player2))
    {
        Float3 itemBox(g_Player2.primaryShtFile->itemCollectionBoxSize,
                       g_Player2.primaryShtFile->itemCollectionBoxSize, 16.0f);
        if (g_Player2.CalcItemBoxCollision(const_cast<Float3 *>(&itemPosition), &itemBox))
            p2 = &g_Player2;
    }
    return ResolveNearestItemPlayer(itemPosition, p1, p2);
}

static bool AllPlayersHaveFullPower()
{
    if (!g_MultiPlayerState.IsEnabled())
        return GetMultiPlayerPower(&g_Player) >= 128;
    return GetMultiPlayerPower(&g_Player) >= 128 &&
           GetMultiPlayerPower(&g_Player2) >= 128;
}

static void CollectExtendForPlayer(Player *collector)
{
    if (GetMultiPlayerLives(collector) < 8)
    {
        AddMultiPlayerLives(collector, 1);
        g_SoundPlayer.PlaySoundByIdx(SOUND_1UP, 0);
        g_GameManager.IncreaseSubrank(200);
        g_Gui.flags.lifeDisplayUpdateFrames = 2;
    }
    else if (GetMultiPlayerBombs(collector) < 8)
    {
        AddMultiPlayerBombs(collector, 1);
        g_SoundPlayer.PlaySoundByIdx(SOUND_1UP, 0);
        g_GameManager.IncreaseSubrank(200);
        g_Gui.flags.bombDisplayUpdateFrames = 2;
    }
}
#endif

// FUNCTION: th08 0x441830
ZunBool ZunTimer::operator!=(int value)
{
    return this->current != value;
}

// FUNCTION: th08 0x440010
ItemManager::ItemManager()
{
}

// FUNCTION: th08 0x4337f0
void ItemManager::Initialize()
{
    memset(this, 0, sizeof(ItemManager));
    this->itemListTail = &this->itemListHead;
}

// FUNCTION: th08 0x440050
Item::Item()
{
}

#pragma var_order(i, item)
Item *ItemManager::SpawnItem(Float3 *position, ItemType itemType, i32 state)
{
    i32 i;
    Item *item = &this->items[this->nextIndex];

    if (position->x < -64.0f || position->x > 448.0f)
    {
        return &this->items[MAX_ITEMS];
    }

#ifdef TH08_MULTI
    if (AllPlayersHaveFullPower() &&
        (itemType == ITEM_POWER_SMALL || itemType == ITEM_POWER_BIG))
#else
    if (g_GameManager.GetPower() >= 128 && (itemType == ITEM_POWER_SMALL || itemType == ITEM_POWER_BIG))
#endif
    {
        itemType = ITEM_POINT_SMALL;
    }
    if (itemType == ITEM_TIME)
    {
        state = ITEM_STATE_TIME_RISING;
    }
    else if (itemType == ITEM_TIME_APEX_AUTOCOLLECT_REQUEST)
    {
        state = ITEM_STATE_TIME_RISING_TO_APEX;
        itemType = ITEM_TIME;
    }

    for (i = 0; i < MAX_ITEMS; i++)
    {
        this->nextIndex++;

        if (item->isInUse)
        {
            if (this->nextIndex >= MAX_ITEMS)
            {
                this->nextIndex = 0;
                item = &this->items[0];
            }
            else
            {
                item++;
            }

            if (itemType == ITEM_TIME)
            {
                return &this->items[MAX_ITEMS];
            }

            continue;
        }

        if (this->nextIndex >= MAX_ITEMS)
        {
            this->nextIndex = 0;
        }

        item->isInUse = true;
        item->currentPosition = *position;
        item->startPositionOrVelocity.x = 0.0f;
        item->startPositionOrVelocity.y = -2.2f;
        item->startPositionOrVelocity.z = 0.0f;
        item->itemType = itemType;
        item->state = state;
        item->timer = 0;

        if (state == ITEM_STATE_DEATH_DROP_SPREAD)
        {
            item->targetPosition.x = g_Rng.GetRandomF32InRange(288.0f) + 48.0f;
            item->targetPosition.y = g_Rng.GetRandomF32InRange(192.0f) - 64.0f;
            item->targetPosition.z = 0.0f;
            item->startPositionOrVelocity = item->currentPosition;
        }
        else if (state == ITEM_STATE_TIME_RISING)
        {
            item->startPositionOrVelocity.y = -2.0f - g_Rng.GetRandomF32InRange(0.2f);
            item->startPositionOrVelocity.x = g_Rng.GetRandomF32SignedInRange(0.6f);

            if (g_Player.playerState == PLAYER_STATE_DYING)
            {
                item->state = ITEM_STATE_DEFAULT;
                item->startPositionOrVelocity.x = 0.0f;
                item->startPositionOrVelocity.y = -0.9f;
                item->startPositionOrVelocity.z = 0.0f;
            }
        }
        // The initialization is duplicated, but this state has a distinct update transition.
        else if (state == ITEM_STATE_TIME_RISING_TO_APEX)
        {
            item->startPositionOrVelocity.y = -2.0f - g_Rng.GetRandomF32InRange(0.2f);
            item->startPositionOrVelocity.x = g_Rng.GetRandomF32SignedInRange(0.6f);

            if (g_Player.playerState == PLAYER_STATE_DYING)
            {
                item->state = ITEM_STATE_DEFAULT;
                item->startPositionOrVelocity.x = 0.0f;
                item->startPositionOrVelocity.y = -0.9f;
                item->startPositionOrVelocity.z = 0.0f;
            }
        }

        g_BulletManager.bulletAnm->SetAndExecuteScriptIdx(&item->sprite, itemType + 61);

        item->sprite.color1.d3dColor = 0xFFFFFFFF;
        item->sprite.zWriteDisabled = true;
        item->isMaxValue = false;
        item->isOnscreen = true;
        this->itemListTail->next = item;
        item->prev = this->itemListTail;
        item->next = NULL;
        this->itemListTail = item;

        break;
    }

    return i < MAX_ITEMS ? item : &this->items[MAX_ITEMS];
}

DIFFABLE_STATIC_ARRAY_ASSIGN(i32, 6, g_PointItemExtendThresholds) = {100, 250, 500, 800, 1100, 9999};
DIFFABLE_STATIC_ARRAY_ASSIGN(i32, 4, g_ExPointItemExtendThresholds) = {200, 666, 9999, 1};

void ItemManager::UpdatePointItemExtendThreshold()
{
    if (g_GameManager.difficulty < EXTRA)
    {
        if (g_GameManager.globals->pointItemExtendsSoFar < 6)
        {
            g_GameManager.globals->nextPointItemExtendThreshold =
                g_PointItemExtendThresholds[g_GameManager.globals->pointItemExtendsSoFar];
        }
        else
        {
            g_GameManager.globals->nextPointItemExtendThreshold =
                (g_GameManager.globals->pointItemExtendsSoFar - 5) * 500 + g_PointItemExtendThresholds[5];
        }
    }
    else
    {
        if (g_GameManager.globals->pointItemExtendsSoFar < 3)
        {
            g_GameManager.globals->nextPointItemExtendThreshold =
                g_ExPointItemExtendThresholds[g_GameManager.globals->pointItemExtendsSoFar];
        }
        else
        {
            g_GameManager.globals->nextPointItemExtendThreshold = 99999;
        }
    }
}

// FUNCTION: th08 0x440500
#pragma var_order(speed, interp, pickupScore, angle, itemBox, soundIndex, item)
void ItemManager::OnUpdate()
{
    f32 speed;
    f32 interp;
    i32 pickupScore;
    f32 angle;
    i32 soundIndex = 0;
    Item *item = this->itemListHead.next;
#ifdef TH08_MULTI
    Player *autoCollectPlayer;
    Player *collector;
#else
    Float3 itemBox(g_Player.primaryShtFile->itemCollectionBoxSize,
                   g_Player.primaryShtFile->itemCollectionBoxSize, 16.0f);
#endif

    this->itemCount = 0;
    speed = g_Player.focusMode ? g_Player.secondaryShtFile->itemMovementSpeed
                                    : g_Player.primaryShtFile->itemMovementSpeed;
    speed *= g_Supervisor.framerateMultiplier;

    while (item != NULL)
    {
        this->itemCount++;

        if (item->state == ITEM_STATE_DEATH_DROP_SPREAD)
        {
            if (item->timer < 60)
            {
                interp = (f32)item->timer / 60.0f;
                item->currentPosition =
                    item->targetPosition * interp + item->startPositionOrVelocity * (1.0f - interp);
                goto pickup;
            }
            if (item->timer == 60)
            {
                item->startPositionOrVelocity = Float3(0.0f, 0.0f, 0.0f);
                item->state = ITEM_STATE_DEFAULT;
            }
            goto moveItem;
        }
        else if (item->state == ITEM_STATE_TIME_RISING)
        {
            item->startPositionOrVelocity.y += 0.05f * g_Supervisor.framerateMultiplier;
            if (item->startPositionOrVelocity.y > 0.0f ||
                g_Player.shotTimer < 0)
            {
                item->state = ITEM_STATE_AUTOCOLLECT;
            }
            if (g_Player.playerState == PLAYER_STATE_DYING)
            {
                item->state = ITEM_STATE_DEFAULT;
                item->startPositionOrVelocity.x = 0.0f;
                item->startPositionOrVelocity.y = -0.7f;
                item->startPositionOrVelocity.z = 0.0f;
            }
            goto moveItem;
        }
        else if (item->state == ITEM_STATE_TIME_RISING_TO_APEX)
        {
            item->startPositionOrVelocity.y += 0.05f * g_Supervisor.framerateMultiplier;
            item->currentPosition += item->startPositionOrVelocity * speed;
            if (item->startPositionOrVelocity.y > 0.0f)
            {
                item->state = ITEM_STATE_AUTOCOLLECT;
            }
            else
            {
                goto executeOnly;
            }
            if (g_Player.playerState == PLAYER_STATE_DYING)
            {
                item->state = ITEM_STATE_DEFAULT;
                item->startPositionOrVelocity.x = 0.0f;
                item->startPositionOrVelocity.y = -0.7f;
                item->startPositionOrVelocity.z = 0.0f;
            }
            goto moveItem;
        }
        else
        {
#ifdef TH08_MULTI
            autoCollectPlayer = item->state == ITEM_STATE_AUTOCOLLECT
                                    ? ResolveNearestPhysicalItemPlayer(item->currentPosition)
                                    : ResolvePointOfCollectionPlayer(item->currentPosition);
            if (autoCollectPlayer != NULL)
            {
                angle = autoCollectPlayer->AngleToPoint(&item->currentPosition);
                item->startPositionOrVelocity.FromAngleMagnitude(
                    angle, autoCollectPlayer->primaryShtFile->itemAutoCollectSpeed);
                item->state = ITEM_STATE_AUTOCOLLECT;
                item->currentPosition += item->startPositionOrVelocity * g_Supervisor.framerateMultiplier;
                goto pickup;
            }
            if (item->state == ITEM_STATE_AUTOCOLLECT)
            {
                item->startPositionOrVelocity.y = -0.7f;
                item->state = ITEM_STATE_DEFAULT;
            }
            else
            {
                item->startPositionOrVelocity.x = 0.0f;
                item->startPositionOrVelocity.z = 0.0f;
                if (item->startPositionOrVelocity.y < -2.2f)
                    item->startPositionOrVelocity.y = -2.2f;
            }
#else
            if (item->state == ITEM_STATE_AUTOCOLLECT ||
                (g_Player.position.y < g_Player.primaryShtFile->pointItemValueLine &&
                 (g_GameManager.GetPower() >= 128.0 ||
                  g_Player.focusMode != PLAYER_FOCUS_MODE_UNFOCUSED ||
                  g_GameManager.shotType == 1 || g_GameManager.shotType == 6)))
            {
                if (g_Player.playerState != PLAYER_STATE_DYING && g_Player.playerState != PLAYER_STATE_SPAWNING)
                {
                    angle = g_Player.AngleToPoint(&item->currentPosition);
                    item->startPositionOrVelocity.FromAngleMagnitude(
                        angle, g_Player.primaryShtFile->itemAutoCollectSpeed);
                    item->state = ITEM_STATE_AUTOCOLLECT;
                    item->currentPosition += item->startPositionOrVelocity * g_Supervisor.framerateMultiplier;
                    goto pickup;
                }
                item->startPositionOrVelocity.y = -0.7f;
                item->state = ITEM_STATE_DEFAULT;
            }
            else
            {
                item->startPositionOrVelocity.x = 0.0f;
                item->startPositionOrVelocity.z = 0.0f;
                if (item->startPositionOrVelocity.y < -2.2f)
                    item->startPositionOrVelocity.y = -2.2f;
            }
#endif
        }

moveItem:
        item->currentPosition += item->startPositionOrVelocity * speed;
        if (item->state == ITEM_STATE_DEFAULT && g_GameManager.arcadeRegionSize.y + 16.0f <= item->currentPosition.y)
        {
            g_GameManager.DecreaseSubrank(3);
            item->Delete();
            item = item->next;
            continue;
        }

        if (item->startPositionOrVelocity.operator float *()[1] < 3.0f)
            item->startPositionOrVelocity.y += 0.03f * speed;
        else
            item->startPositionOrVelocity.y = 3.0f;

pickup:
#ifdef TH08_MULTI
        collector = item->state != ITEM_STATE_TIME_RISING
                        ? ResolveCollidingItemPlayer(item->currentPosition)
                        : NULL;
        if (collector != NULL)
#else
        if (item->state != ITEM_STATE_TIME_RISING &&
            g_Player.CalcItemBoxCollision(&item->currentPosition, &itemBox))
#endif
        {
            g_ReplayManager->frameEventFlags |= REPLAY_FRAME_EVENT_ITEM_COLLECTED;
            switch (item->itemType)
            {
            case ITEM_POWER_SMALL:
#ifdef TH08_MULTI
                if (GetMultiPlayerPower(collector) >= 128)
                    item->CollectPointSmall(collector);
                else
                    item->CollectPowerSmall(collector);
#else
                item->CollectPowerSmall();
#endif
                break;
            case ITEM_POINT:
#ifdef TH08_MULTI
                item->CollectPoint(collector);
#else
                item->CollectPoint();
#endif
                break;
            case ITEM_POINT_SMALL:
#ifdef TH08_MULTI
                item->CollectPointSmall(collector);
#else
                item->CollectPointSmall();
#endif
                break;
            case ITEM_POWER_BIG:
#ifdef TH08_MULTI
                if (GetMultiPlayerPower(collector) >= 128)
                    item->CollectPointSmall(collector);
                else
                    item->CollectPowerBig(collector);
#else
                item->CollectPowerBig();
#endif
                break;
            case ITEM_BOMB:
#ifdef TH08_MULTI
                if (GetMultiPlayerBombs(collector) < 8)
                {
                    AddMultiPlayerBombs(collector, 1);
                    g_Gui.flags.bombDisplayUpdateFrames = 2;
                }
#else
                if (g_GameManager.GetBombsRemaining() < 8)
                {
                    g_GameManager.AddToBombCount(1);
                    g_Gui.flags.bombDisplayUpdateFrames = 2;
                }
#endif
                g_GameManager.IncreaseSubrank(5);
                break;
            case ITEM_EXTEND:
#ifdef TH08_MULTI
                CollectExtendForPlayer(collector);
#else
                g_GameManager.CollectExtend();
#endif
                break;
            case ITEM_POWER_FULL:
#ifdef TH08_MULTI
                if (GetMultiPlayerPower(collector) < 128)
                {
                    g_BulletManager.ClearBulletsForTransition();
                    g_Gui.ShowPopupText(0, 1);
                    g_SoundPlayer.PlaySoundByIdx(SOUND_POWERUP, 0);
                    g_AsciiManager.CreatePlayerPointPopup(&item->currentPosition, -1, 0xffffc0a0);
                }
                SetMultiPlayerPower(collector, 128);
                if (AllPlayersHaveFullPower())
                    this->ConvertAllPowerItemsToTimeOrbs(item);
#else
                if (g_GameManager.GetPower() < 128)
                {
                    g_BulletManager.ClearBulletsForTransition();
                    g_Gui.ShowPopupText(0, 1);
                    g_SoundPlayer.PlaySoundByIdx(SOUND_POWERUP, 0);
                    g_AsciiManager.CreatePlayerPointPopup(&item->currentPosition, -1, 0xffffc0a0);
                    this->ConvertAllPowerItemsToTimeOrbs(item);
                }
                g_GameManager.SetPower(128);
#endif
                g_GameManager.AddScore(1000);
                g_AsciiManager.CreatePlayerPointPopup(&item->currentPosition, 1000, 0xffffffff);
                g_Gui.flags.powerDisplayUpdateFrames = 2;
                break;
            case ITEM_POINT_STAR:
#ifdef TH08_MULTI
                if (collector->itemTimeOrbMode == 0)
                {
                    pickupScore = (GetMultiPlayerGraze(collector) / 40) * 10 + 300;
                    if (pickupScore <= 0)
                        pickupScore = 10;
                }
                else
                {
                    pickupScore = 100;
                }
#else
                if (g_Player.itemTimeOrbMode == 0)
                {
                    pickupScore = (g_GameManager.globals->graze / 40) * 10 + 300;
                    if (pickupScore <= 0)
                        pickupScore = 10;
                }
                else
                {
                    pickupScore = 100;
                }
#endif
                g_AsciiManager.CreateScorePopup(&item->currentPosition, pickupScore, 0xffffffff);
                g_GameManager.AddScore(pickupScore);
                break;
            case ITEM_TIME:
#ifdef TH08_MULTI
                item->CollectTimeOrb(collector);
#else
                item->CollectTimeOrb();
#endif
                break;
            default:
                break;
            }

            if (soundIndex <= SOUND_ITEM)
                soundIndex = item->isMaxValue ? SOUND_ITEM_MAX_VALUE : SOUND_ITEM;
            item->Delete();
            item = item->next;
            continue;
        }

executeOnly:
        item->timer++;
        if (item->sprite.currentInstruction != NULL)
            g_AnmManager->ExecuteScript(&item->sprite);
        item = item->next;
    }

    if (soundIndex != 0)
        g_SoundPlayer.PlaySoundByIdx((SoundIdx)soundIndex, 0);

#ifdef TH08_MULTI
    Player *players[MULTI_PLAYER_COUNT] = {&g_Player, &g_Player2};
    for (i32 playerIndex = 0; playerIndex < MULTI_PLAYER_COUNT; playerIndex++)
    {
        Player *player = players[playerIndex];
        if (player->timeOrbGaugeChangeSuppressionTimer != 0)
        {
            player->timeOrbGaugeChangeSuppressionTimer--;
            if (player->timeOrbGaugeChangeSuppressionTimer <= 0)
                player->timeOrbGaugeChangeSuppressionTimer = 0;
        }
    }
#else
    if (g_Player.timeOrbGaugeChangeSuppressionTimer != 0)
    {
        g_Player.timeOrbGaugeChangeSuppressionTimer--;
        if (g_Player.timeOrbGaugeChangeSuppressionTimer <= 0)
            g_Player.timeOrbGaugeChangeSuppressionTimer = 0;
    }
#endif
}

// FUNCTION: th08 0x440cf0
#pragma var_order(powerLevel, oldPowerLevel)
#ifdef TH08_MULTI
void Item::CollectPowerSmall(Player *collector)
#else
void Item::CollectPowerSmall()
#endif
{
    i32 powerLevel;
    i32 oldPowerLevel;

#ifdef TH08_MULTI
    if (GetMultiPlayerPower(collector) >= 0x80)
#else
    if (g_GameManager.GetPower() >= 0x80)
#endif
    {
        goto increaseSubrank;
    }

    powerLevel = 0;
#ifdef TH08_MULTI
    while (GetMultiPlayerPower(collector) >= g_PowerUpThresholds[powerLevel])
#else
    while (g_GameManager.GetPower() >= g_PowerUpThresholds[powerLevel])
#endif
    {
        powerLevel++;
    }
    oldPowerLevel = powerLevel;

    g_GameManager.character = 0;
#ifdef TH08_MULTI
    AddMultiPlayerPower(collector, 1);
#else
    g_GameManager.AddPower(1);
#endif

#ifdef TH08_MULTI
    if (GetMultiPlayerPower(collector) >= 0x80)
#else
    if (g_GameManager.GetPower() >= 0x80)
#endif
    {
#ifdef TH08_MULTI
        SetMultiPlayerPower(collector, 0x80);
#else
        g_GameManager.SetPower(0x80);
#endif
        if (!g_Spellcard.IsActive())
        {
            g_BulletManager.ClearBulletsForTransition();
        }
        g_Gui.ShowPopupText(0, 1);
#ifdef TH08_MULTI
        if (AllPlayersHaveFullPower())
            g_ItemManager.ConvertAllPowerItemsToTimeOrbs(this);
#else
        g_ItemManager.ConvertAllPowerItemsToTimeOrbs(this);
#endif
    }

    g_GameManager.AddScore(10);
    g_Gui.flags.powerDisplayUpdateFrames = 2;

#ifdef TH08_MULTI
    while (GetMultiPlayerPower(collector) >= g_PowerUpThresholds[powerLevel])
#else
    while (g_GameManager.GetPower() >= g_PowerUpThresholds[powerLevel])
#endif
    {
        powerLevel++;
    }

    if (powerLevel != oldPowerLevel)
    {
        g_AsciiManager.CreateScorePopup(&this->currentPosition, -1, 0xffffc0a0);
        g_SoundPlayer.PlaySoundByIdx(SOUND_POWERUP, 0);
    }
    else
    {
        g_AsciiManager.CreateScorePopup(&this->currentPosition, 10, 0xffffffff);
    }

increaseSubrank:
    g_GameManager.IncreaseSubrank(1);
}

// FUNCTION: th08 0x440e40
#pragma var_order(pointItemValueBase, currentPointItemValue)
#ifdef TH08_MULTI
void Item::CollectPoint(Player *collector)
#else
void Item::CollectPoint()
#endif
{
    i32 pointItemValueBase = g_GameManager.globals->pointItemValue;
    i32 currentPointItemValue;

#ifdef TH08_MULTI
    currentPointItemValue = static_cast<ZunBool>(this->currentPosition.y < collector->primaryShtFile->pointItemValueLine)
                                ? pointItemValueBase
                                : pointItemValueBase / 2 -
                                      (i32)(this->currentPosition.y - collector->primaryShtFile->pointItemValueLine) *
                                          (g_GameManager.globals->pointItemValue / 1000);
#else
    currentPointItemValue = static_cast<ZunBool>(this->currentPosition.y < g_Player.primaryShtFile->pointItemValueLine)
                                ? pointItemValueBase
                                : pointItemValueBase / 2 -
                                      (i32)(this->currentPosition.y - g_Player.primaryShtFile->pointItemValueLine) *
                                          (g_GameManager.globals->pointItemValue / 1000);
#endif
    if (this->isMaxValue == 1)
    {
        currentPointItemValue = pointItemValueBase;
    }

    currentPointItemValue -= currentPointItemValue % 10;
#ifdef TH08_MULTI
    if (MultiPlayerGaugeIsExtremelyHuman(collector))
#else
    if (g_GameManager.GaugeIsExtremelyHuman())
#endif
    {
        currentPointItemValue += currentPointItemValue;
    }

    g_AsciiManager.CreateScorePopup(&this->currentPosition, currentPointItemValue,
                                    currentPointItemValue >= pointItemValueBase ? 0xffffff00 : 0xffffffff);
    if (currentPointItemValue >= pointItemValueBase)
    {
        this->isMaxValue = true;
    }

    g_GameManager.AddScore(currentPointItemValue);
    g_GameManager.globals->pointItemsCollectedInStage++;
    g_GameManager.globals->pointItemsCollected++;
    g_Gui.flags.pointDisplayUpdateFrames = 2;

    if (currentPointItemValue >= pointItemValueBase)
    {
        g_GameManager.IncreaseSubrank(10);
    }
    else
    {
        g_GameManager.IncreaseSubrank(3);
    }

    if ((i32)g_GameManager.globals->pointItemExtendsSoFar >= 0)
    {
        while ((ItemManager::UpdatePointItemExtendThreshold(),
                g_GameManager.globals->pointItemsCollected >= g_GameManager.globals->nextPointItemExtendThreshold))
        {
#ifdef TH08_MULTI
            CollectExtendForPlayer(collector);
#else
            g_GameManager.CollectExtend();
#endif
            g_GameManager.globals->pointItemExtendsSoFar++;
        }
    }

    g_MaxValuePointItemsCollected++;
    g_GameManager.UpdateAntiTamper();
}

// FUNCTION: th08 0x441020
#pragma var_order(pointItemValueBase, currentPointItemValue)
#ifdef TH08_MULTI
void Item::CollectPointSmall(Player *collector)
#else
void Item::CollectPointSmall()
#endif
{
    i32 pointItemValueBase = g_GameManager.globals->pointItemValue;
    i32 currentPointItemValue;

#ifdef TH08_MULTI
    currentPointItemValue = static_cast<ZunBool>(this->currentPosition.y < collector->primaryShtFile->pointItemValueLine)
                                ? pointItemValueBase
                                : pointItemValueBase / 2 -
                                      (i32)(this->currentPosition.y - collector->primaryShtFile->pointItemValueLine) *
                                          (g_GameManager.globals->pointItemValue / 1000);
#else
    currentPointItemValue = static_cast<ZunBool>(this->currentPosition.y < g_Player.primaryShtFile->pointItemValueLine)
                                ? pointItemValueBase
                                : pointItemValueBase / 2 -
                                      (i32)(this->currentPosition.y - g_Player.primaryShtFile->pointItemValueLine) *
                                          (g_GameManager.globals->pointItemValue / 1000);
#endif
    if (this->isMaxValue == 1)
    {
        currentPointItemValue = pointItemValueBase;
    }

    pointItemValueBase /= 10;
    pointItemValueBase -= pointItemValueBase % 10;
    currentPointItemValue /= 10;
    currentPointItemValue -= currentPointItemValue % 10;
#ifdef TH08_MULTI
    if (MultiPlayerGaugeIsExtremelyHuman(collector))
#else
    if (g_GameManager.GaugeIsExtremelyHuman())
#endif
    {
        currentPointItemValue += currentPointItemValue;
    }

    g_AsciiManager.CreateScorePopup(&this->currentPosition, currentPointItemValue,
                                    currentPointItemValue >= pointItemValueBase ? 0xffffff00 : 0xffffffff);
    g_GameManager.AddScore(currentPointItemValue);
    if (currentPointItemValue >= pointItemValueBase)
    {
        this->isMaxValue = true;
    }
}

// FUNCTION: th08 0x441170
#pragma var_order(powerLevel, oldPowerLevel)
#ifdef TH08_MULTI
void Item::CollectPowerBig(Player *collector)
#else
void Item::CollectPowerBig()
#endif
{
    i32 powerLevel;
    i32 oldPowerLevel;

#ifdef TH08_MULTI
    if (GetMultiPlayerPower(collector) >= 0x80)
#else
    if (g_GameManager.GetPower() >= 0x80)
#endif
    {
        return;
    }

    powerLevel = 0;
#ifdef TH08_MULTI
    while (GetMultiPlayerPower(collector) >= g_PowerUpThresholds[powerLevel])
#else
    while (g_GameManager.GetPower() >= g_PowerUpThresholds[powerLevel])
#endif
    {
        powerLevel++;
    }
    oldPowerLevel = powerLevel;

#ifdef TH08_MULTI
    AddMultiPlayerPower(collector, 8);
#else
    g_GameManager.AddPower(8);
#endif

#ifdef TH08_MULTI
    if (GetMultiPlayerPower(collector) >= 0x80)
#else
    if (g_GameManager.GetPower() >= 0x80)
#endif
    {
#ifdef TH08_MULTI
        SetMultiPlayerPower(collector, 0x80);
#else
        g_GameManager.SetPower(0x80);
#endif
        if (!g_Spellcard.IsActive())
        {
            g_BulletManager.ClearBulletsForTransition();
        }
        g_Gui.ShowPopupText(0, 1);
#ifdef TH08_MULTI
        if (AllPlayersHaveFullPower())
            g_ItemManager.ConvertAllPowerItemsToTimeOrbs(this);
#else
        g_ItemManager.ConvertAllPowerItemsToTimeOrbs(this);
#endif
    }

    g_Gui.flags.powerDisplayUpdateFrames = 2;
    g_GameManager.AddScore(10);

#ifdef TH08_MULTI
    while (GetMultiPlayerPower(collector) >= g_PowerUpThresholds[powerLevel])
#else
    while (g_GameManager.GetPower() >= g_PowerUpThresholds[powerLevel])
#endif
    {
        powerLevel++;
    }

    if (powerLevel != oldPowerLevel)
    {
        g_AsciiManager.CreateScorePopup(&this->currentPosition, -1, 0xffffc0a0);
        g_SoundPlayer.PlaySoundByIdx(SOUND_POWERUP, 0);
    }
    else
    {
        g_AsciiManager.CreateScorePopup(&this->currentPosition, 10, 0xffffffff);
    }
}

// FUNCTION: th08 0x4412b0
#pragma var_order(score)
#ifdef TH08_MULTI
void Item::CollectTimeOrb(Player *collector)
#else
void Item::CollectTimeOrb()
#endif
{
    i32 score;

#ifdef TH08_MULTI
    if (collector->itemTimeOrbMode == 0)
#else
    if (g_Player.itemTimeOrbMode == 0)
#endif
    {
        if (g_GameManager.globals->pointItemsCollectedInStage >= 2000)
        {
            score = 10000;
        }
        else
        {
            score = (g_GameManager.globals->pointItemsCollected / 2) * 10;
            if (score < 100)
                score = 100;
        }
    }
    else
    {
        score = 100;
    }

    if (this != NULL)
    {
        g_AsciiManager.CreateScorePopup(
            &this->currentPosition, score,
            g_GameManager.GetTimeOrbs() < g_GameManager.GetLastSpellTimeOrbThreshold() ? -536870913 : -536875136);
    }

    g_Gui.flags.timeDisplayUpdateFrames = 2;
    g_GameManager.AddScore(score);
    g_GameManager.AddTimeOrbs(1);
    g_Spellcard.AddBonusProgress(8000);

#ifdef TH08_MULTI
    if (collector->timeOrbGaugeChangeSuppressionTimer == 0)
#else
    if (g_Player.timeOrbGaugeChangeSuppressionTimer == 0)
#endif
    {
        score = 111;
#ifdef TH08_MULTI
        AddMultiPlayerYoukaiGauge(
            collector, collector->focusMode ? score : -score, 0);
#else
        g_GameManager.AddToYoukaiGauge(
            g_Player.focusMode ? score : -score, 0);
#endif
    }
}

// FUNCTION: th08 0x4413e0
void ItemManager::AutoCollectAllItems()
{
    Item *item = this->itemListHead.next;
    while (item != NULL)
    {
        item->state = ITEM_STATE_AUTOCOLLECT;
        item->startPositionOrVelocity = Float3(0.0f, -0.5f, 0.0f);
        item = item->next;
    }
}

// FUNCTION: th08 0x441450
void ItemManager::ConvertAllPowerItemsToTimeOrbs(Item *item)
{
    Item *current = this->itemListHead.next;

    while (current != NULL)
    {
        if (current != item && (current->itemType == ITEM_POWER_SMALL || current->itemType == ITEM_POWER_BIG))
        {
            if (current->startPositionOrVelocity.y > -0.5f)
            {
                current->startPositionOrVelocity.x = 0.0f;
                current->startPositionOrVelocity.y = -0.5f;
                current->startPositionOrVelocity.z = 0.0f;
            }
            g_EffectManager.SpawnEffect(EFFECT_ANM_SCRIPT_28, D3DXVECTOR3_PTR(&current->currentPosition), 1, -1);
            current->itemType = ITEM_POINT_SMALL;
            g_BulletManager.bulletAnm->SetAndExecuteScriptIdx(&current->sprite, ITEM_POINT_SMALL + 61);
        }
        current = current->next;
    }
}

// FUNCTION: th08 0x441530
void ItemManager::CancelAutoCollect()
{
    Item *item = this->itemListHead.next;
    while (item != NULL)
    {
        if (item->state == ITEM_STATE_AUTOCOLLECT)
        {
            item->state = ITEM_STATE_DEFAULT;
            item->startPositionOrVelocity.x = 0.0f;
            item->startPositionOrVelocity.y = -0.9f;
            item->startPositionOrVelocity.z = 0.0f;
        }
        item = item->next;
    }
}

// FUNCTION: th08 0x4415a0
#pragma var_order(alpha, item, this)
void ItemManager::OnDraw()
{
    i32 alpha;
    Item *item = this->itemListHead.next;

    while (item != NULL)
    {
        item->sprite.pos.x = g_GameManager.arcadeRegionTopLeftPos.x + item->currentPosition.x;
        item->sprite.pos.y = g_GameManager.arcadeRegionTopLeftPos.y + item->currentPosition.y;
        item->sprite.pos.z = 0.15f;

        // Keep the target's Float3::operator float *() call shape: direct .y
        // access removes both calls and changes the VC7 function extent.
        if (((f32 *)item->currentPosition)[1] < -8.0f)
        {
            item->sprite.pos.y = 8.0f + g_GameManager.arcadeRegionTopLeftPos.y;
            if (item->isOnscreen)
            {
                g_BulletManager.bulletAnm->SetSprite(&item->sprite, item->itemType + 0xb6);
                item->isOnscreen = false;
                item->sprite.zWriteDisabled = true;
            }

            alpha = 255 - (i32)(((8.0f - ((f32 *)item->currentPosition)[1]) * 255.0f) / 128.0f);
            if (alpha < 0x40)
            {
                alpha = 0x40;
            }
            item->sprite.color1.d3dColor = (item->sprite.color1.d3dColor & 0xffffff) | (alpha << 24);
        }
        else
        {
            if (!item->isOnscreen)
            {
                g_BulletManager.bulletAnm->SetSprite(&item->sprite, item->itemType + 0xac);
                item->isOnscreen = true;
                item->sprite.color1.d3dColor = 0xffffffff;
                item->sprite.zWriteDisabled = true;
            }
        }

        g_AnmManager->Draw2D(&item->sprite);
        item = item->next;
    }
}

void Item::Delete()
{
    this->isInUse = false;
    this->prev->next = this->next;
    if (this->next != NULL)
    {
        this->next->prev = this->prev;
    }
    if (g_ItemManager.itemListTail == this)
    {
        g_ItemManager.itemListTail = this->prev;
    }
}

i32 ItemManager::GetTimeOrbCount()
{
    Item *next = this->itemListHead.next;
    i32 count = 0;

    while (next != NULL)
    {
        if (next->itemType == ITEM_TIME)
        {
            count++;
        }
        next = next->next;
    }

    return count;
}

} /* namespace th08 */
