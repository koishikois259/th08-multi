#include "ItemManager.hpp"
#include "GameManager.hpp"

namespace th08
{

DIFFABLE_STATIC(ItemManager, g_ItemManager);

Item *ItemManager::SpawnItem(D3DXVECTOR3 *position, i32 itemType, i32 state)
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

void ItemManager::ConventAllPowerItemsToTimeOrbs(Item *item)
{
}

void ItemManager::CancelAutoCollect()
{
}

void ItemManager::OnDraw()
{
}

void Item::Delete()
{
}

i32 ItemManager::GetTimeOrbCount()
{
    return 0;
}

} /* namespace th08 */