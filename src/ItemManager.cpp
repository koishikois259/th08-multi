#include "ItemManager.hpp"
#include "GameManager.hpp"

namespace th08
{

DIFFABLE_STATIC(ItemManager, g_ItemManager);

Item *ItemManager::SpawnItem(D3DXVECTOR3 *position, ItemType itemType, i32 state)
{
    return NULL;
}

DIFFABLE_STATIC_ARRAY_ASSIGN(i32, 6, g_PointItemExtendThresholds) = {100, 250, 500, 800, 1100, 9999};
DIFFABLE_STATIC_ARRAY_ASSIGN(i32, 4, g_ExPointItemExtendThresholds) = {200, 666, 9999, 1};

void ItemManager::UpdatePointItemExtendThreshold()
{
    if (g_GameManager.difficulty < 4)
    {
        if (g_GameManager.globals->pointItemExtendsSoFar < 6)
        {
            g_GameManager.globals->nextPointItemExtendThreshold = g_PointItemExtendThresholds[g_GameManager.globals->pointItemExtendsSoFar];
        }
        else
        {
            g_GameManager.globals->nextPointItemExtendThreshold = (g_GameManager.globals->pointItemExtendsSoFar - 5) * 500 + g_PointItemExtendThresholds[5];
        }
    }
    else
    {
        if (g_GameManager.globals->pointItemExtendsSoFar < 3)
        {
            g_GameManager.globals->nextPointItemExtendThreshold = g_ExPointItemExtendThresholds[g_GameManager.globals->pointItemExtendsSoFar];
        }
        else
        {
            g_GameManager.globals->nextPointItemExtendThreshold = 99999;
        }
    }
}

void ItemManager::OnUpdate()
{
    // TODO: NEEDS WORK ON Gui
}

void Item::CollectPowerSmall()
{
}

void Item::CollectPoint()
{
}

void Item::CollectPointSmall()
{
}

void Item::CollectPowerBig()
{
}

void Item::CollectTimeOrb()
{
}

void ItemManager::AutoCollectAllItems()
{
}

void ItemManager::ConvertAllPowerItemsToTimeOrbs(Item *item)
{
    // TODO: NEEDS WORK ON EffectManager
}

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

void ItemManager::OnDraw()
{
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