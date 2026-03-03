#include "AnmManager.hpp"

namespace th08
{
DIFFABLE_STATIC(AnmManager *, g_AnmManager);

void AnmManager::ExecuteScriptOnVmArray(AnmVm *sprite, int count)
{
    while(count != 0)
    {
        if (sprite->scriptIndex >= 0)
        {
            g_AnmManager->ExecuteScript(sprite);
        }
        sprite++;
        count--;
    }
}

}; // Namespace th08
