#include "AsciiManager.hpp"
#include "AnmManager.hpp"

namespace th08
{

DIFFABLE_STATIC(ChainElem, g_AsciiManagerDrawChainLowPrio);
DIFFABLE_STATIC(AsciiManager, g_AsciiManager);
DIFFABLE_STATIC(ChainElem, g_AsciiManagerCalcChain);
DIFFABLE_STATIC(ChainElem, g_AsciiManagerDrawChainHighPrio);

ChainCallbackResult AsciiManager::OnUpdate(AsciiManager *ascii)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult AsciiManager::OnDrawLowPrio(AsciiManager *ascii)
{
    ascii->OnDrawLowPrioImpl();
    ascii->ResetStrings();
    ascii->pauseMenu.OnDraw();
    ascii->retryMenu.OnDraw();
    if (ascii->demoIcon.scriptIndex != 0)
    {
        g_AnmManager->DrawNoRotation(&ascii->demoIcon);
    }

    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult AsciiManager::OnDrawHighPrio(AsciiManager *ascii)
{
    ascii->OnDrawHighPrioImpl();

    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

void AsciiManager::Reset()
{
}

void AsciiManager::InitializeVms()
{
}

ZunResult AsciiManager::RegisterChain()
{
    AsciiManager *ascii = &g_AsciiManager;

    g_AsciiManagerCalcChain.SetCallback((ChainCallback) AsciiManager::OnUpdate);
    g_AsciiManagerCalcChain.addedCallback = (ChainLifetimeCallback) AsciiManager::AddedCallback;
    g_AsciiManagerCalcChain.deletedCallback = (ChainLifetimeCallback) AsciiManager::DeletedCallback;
    g_AsciiManagerCalcChain.arg = ascii;
    if (g_Chain.AddToCalcChain(&g_AsciiManagerCalcChain, 1) != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    g_AsciiManagerDrawChainLowPrio.SetCallback((ChainCallback) AsciiManager::OnDrawLowPrio);
    g_AsciiManagerDrawChainLowPrio.arg = ascii;
    g_Chain.AddToDrawChain(&g_AsciiManagerDrawChainLowPrio, 20);

    g_AsciiManagerDrawChainHighPrio.SetCallback((ChainCallback) AsciiManager::OnDrawHighPrio);
    g_AsciiManagerDrawChainHighPrio.arg = ascii;
    g_Chain.AddToDrawChain(&g_AsciiManagerDrawChainHighPrio, 14);

    return ZUN_SUCCESS;
}

ZunResult AsciiManager::AddedCallback(AsciiManager *ascii)
{
    memset(ascii, 0, sizeof(AsciiManager));

    ascii->asciiAnm = g_AnmManager->PreloadAnm(1, "ascii.anm");
    if (ascii->asciiAnm == NULL)
    {
        return ZUN_ERROR;
    }

    ascii->captureAnm = g_AnmManager->PreloadAnm(3, "capture.anm");
    if (ascii->captureAnm == NULL)
    {
        return ZUN_ERROR;
    }

    ascii->Reset();
    ascii->InitializeVms();

    return ZUN_SUCCESS;
}

ZunResult AsciiManager::DeletedCallback(AsciiManager *ascii)
{
    return ZUN_ERROR;
}

void AsciiManager::CutChain()
{
}

void AsciiManager::AddString(D3DXVECTOR3 *position, const char *string)
{
}

void AsciiManager::AddFormatString(D3DXVECTOR3 *position, const char *fmt, ...)
{
}

void AsciiManager::AddFormatString2(D3DXVECTOR3 *position, const char *fmt, ...)
{
}

void AsciiManager::OnDrawLowPrioImpl()
{

}

void AsciiManager::CreateScorePopup(D3DXVECTOR3 *position, i32 number, D3DCOLOR color)
{
}

void AsciiManager::CreatePlayerPointPopup(D3DXVECTOR3 *position, i32 number, D3DCOLOR color)
{
}

void AsciiManager::CreateTimePopup(D3DXVECTOR3 *position, i32 number, i32 param3, D3DCOLOR color)
{
}

void AsciiManager::CreateFamiliarPopup(D3DXVECTOR3 *position, i32 param1, i32 param2)
{
}

i32 PauseMenu::OnUpdate()
{
    return 0;
}

i32 PauseMenu::OnDraw()
{
    return 0;
}

i32 RetryMenu::OnUpdate()
{
    return 0;
}

i32 RetryMenu::OnDraw()
{
    return 0;
}

void AsciiManager::OnDrawHighPrioImpl()
{
}

void AsciiManager::DrawPercentage(i32 param1, i32 param2, D3DCOLOR color)
{
}

void AsciiManager::UpdateVms()
{
}

void AsciiManager::SetGaugeInterrupt(i32 interrupt)
{
    this->youkaiGauge.SetInterrupt(interrupt);
    this->youkaiGaugeHumanIcon.SetInterrupt(interrupt);
    this->youkaiGaugeYoukaiIcon.SetInterrupt(interrupt);
    this->youkaiGaugeCursor.SetInterrupt(interrupt);

    this->gaugeInterrupt = interrupt;
}

i32 AsciiManager::GetGaugeInterrupt()
{
    return this->gaugeInterrupt;
}

void AsciiManager::ResetStrings()
{
    this->numStrings = 0;
}

void AsciiManager::SetSpaceWidth(i32 spaceWidth)
{
    this->spaceWidth = spaceWidth;
}

} /* namespace th08 */