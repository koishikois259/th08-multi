#pragma once
#include "ZunResult.hpp"
#include "diffbuild.hpp"
#include "Global.hpp"

namespace th08
{

struct Enemy
{
};

struct EnemyManager
{
    void Initialize();
    static ZunResult RegisterChain();
    static ChainCallbackResult OnUpdate();
    static ChainCallbackResult OnDrawHighPrio(EnemyManager *enemyManager);
    ChainCallbackResult OnDrawImpl();
    static ChainCallbackResult OnDrawLowPrio(EnemyManager *enemyManager);
    static ZunResult AddedCallback(EnemyManager *enemyManager);
    static ZunResult DeletedCallback(EnemyManager *enemyManager);
    static void CutChain();
};

DIFFABLE_EXTERN(EnemyManager, g_EnemyManager);

} /* namespace th08 */