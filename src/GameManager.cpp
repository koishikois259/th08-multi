#include "GameManager.hpp"
#include "Global.hpp"

namespace th08
{

DIFFABLE_STATIC(GameManager, g_GameManager);
DIFFABLE_STATIC(ChainElem, g_GameManagerCalcChain);
DIFFABLE_STATIC(ChainElem, g_GameManagerDrawChain);

// STUB: th08 0x4399ac
ZunBool GameManager::IsWithinPlayfield()
{
    return FALSE;
}

i32 GameManager::CalcAntiTamperChecksum()
{
    i32 sum;

    sum = CalcChecksum((u8 *)&g_GameManager.globals->rng1,
                       (intptr_t)&globals->antiTamperValue - (intptr_t)&globals->rng1);
    sum += CalcChecksum((u8 *)&g_GameManager.globals->rng8, sizeof(g_GameManager.globals->rng8));
    sum += CalcChecksum((u8 *)g_GameManager.cfg, sizeof(GameConfiguration));
    sum += CalcChecksum((u8 *)&g_Supervisor.cfg, sizeof(GameConfiguration));
    sum += CalcChecksum((u8 *)&this->hscr, sizeof(Hscr));

    return sum;
}

i32 GameManager::CalcChecksum(u8 *address, i32 size)
{
    i32 sum;
    i32 i;

    for (sum = 0, i = 0; i < size; i++, address++)
    {
        sum += *address;
        g_GameManager.globals->antiTamperValue += g_GameManager.globals->rng8[2];
    }

    return sum;
}

// STUB: th08 0x439b29
void GameManager::CollectExtend()
{
}

// STUB: th08 0x439bc7
ChainCallbackResult GameManager::OnUpdate(GameManager *gameManager)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// STUB: th08 0x43aa03
ChainCallbackResult GameManager::OnDraw(GameManager *gameManager)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// STUB: th08 0x43aa5c
ZunResult GameManager::RegisterChain()
{
    return ZUN_SUCCESS;
}

// STUB: th08 0x43aaf4
ZunResult GameManager::AddedCallback(GameManager *gameManager)
{
    return ZUN_SUCCESS;
}

// STUB: th08 0x43abd7
void GameManager::GameplaySetupThread()
{
}

#pragma var_order(sum, i)
void GameManager::InitializeAntiTamper()
{
    i32 sum;
    u32 i;

    g_GameManager.globals->rng6 = g_Rng.GetRandomU32InRange(100000) + 6543;
    for (i = 0; i < 7; i++)
    {
        g_GameManager.globals->rng1[i] = g_Rng.GetRandomU32InRange(100000) + 6543;
    }
    for (i = 0; i < 8; i++)
    {
        g_GameManager.globals->rng7[i] = g_Rng.GetRandomU32InRange(100000) + 6543;
    }
    for (i = 0; i < 2; i++)
    {
        g_GameManager.globals->rng2[i] = g_Rng.GetRandomF32InRange(100000) + 6543;
    }
    for (i = 0; i < 2; i++)
    {
        g_GameManager.globals->rng3[i] = g_Rng.GetRandomF32InRange(100000) + 6543;
    }
    for (i = 0; i < 3; i++)
    {
        g_GameManager.globals->rng4[i] = g_Rng.GetRandomF32InRange(100000) + 6543;
    }
    for (i = 0; i < 2; i++)
    {
        g_GameManager.globals->rng5[i] = g_Rng.GetRandomF32InRange(100000) + 6543;
    }
    for (i = 0; i < 5; i++)
    {
        g_GameManager.globals->rng8[i] = g_Rng.GetRandomU32InRange(100000) + 6543;
    }
    g_GameManager.globals->antiTamperValue = g_GameManager.globals->rng1[2];
    sum = g_GameManager.CalcAntiTamperChecksum();
    g_GameManager.globals->antiTamperChecksum = sum;
    g_GameManager.antiTamperExpectedValue = (f32)sum + (f32)g_GameManager.globals->rng7[3];
}

// STUB: th08 0x43be2c
ZunResult GameManager::DeletedCallback(GameManager *gameManager)
{
    return ZUN_SUCCESS;
}

// STUB: th08 0x43bfc3
void GameManager::IncreaseSubrank(int amount)
{
}

// STUB: th08 0x43c03f
void GameManager::DecreaseSubrank(int amount)
{
}

// STUB: th08 0x43c0bb
void GameManager::AddToYoukaiGauge(u16 param_1, i32 param_2)
{
}

ZunBool GameManager::IsPhantasmUnlocked()
{
    return FALSE;
}

void GameManager::CutChain()
{
    g_Chain.Cut(&g_GameManagerCalcChain);
    g_Chain.Cut(&g_GameManagerDrawChain);
    if (g_GameManager.globals->score >= 1000000000)
    {
        g_GameManager.globals->score = 999999999;
    }
    g_GameManager.globals->displayScore = g_GameManager.globals->score;
    g_Supervisor.framerateMultiplier = 1.0f;
}

// STUB: th08 0x43c35f
i32 GameManager::GetClockTimeIncrement()
{
    return 0;
}

// STUB: th08 0x43c4b3
void GameManager::AdvanceToNextStage()
{
}

// STUB: th08 0x43c5e1
GameManager::GameManager()
{
    memset(this, 0, sizeof(GameManager));
}

// STUB: th08 0x43c686
void GameManager::InitArcadeRegionParams()
{
}

}; // Namespace th08
