#include "EnemyManager.hpp"

namespace th08
{

DIFFABLE_STATIC(EnemyManager, g_EnemyManager);
DIFFABLE_STATIC(ChainElem, g_EnemyManagerCalcChain);
DIFFABLE_STATIC(ChainElem, g_EnemyManagerDrawChainHighPrio);
DIFFABLE_STATIC(ChainElem, g_EnemyManagerDrawChainLowPrio);

void EnemyManager::Initialize()
{
}

ZunResult EnemyManager::RegisterChain()
{
    return ZUN_ERROR;
}

ChainCallbackResult EnemyManager::OnUpdate()
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult EnemyManager::OnDrawHighPrio(EnemyManager *enemyManager)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult EnemyManager::OnDrawImpl()
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult EnemyManager::OnDrawLowPrio(EnemyManager *enemyManager)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ZunResult EnemyManager::AddedCallback(EnemyManager *enemyManager)
{
    return ZUN_ERROR;
}

ZunResult EnemyManager::DeletedCallback(EnemyManager *enemyManager)
{
    return ZUN_ERROR;
}

void EnemyManager::CutChain()
{
}

} /* namespace th08 */