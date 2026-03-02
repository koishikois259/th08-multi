#include "AnmManager.hpp"

namespace th08
{
DIFFABLE_STATIC(AnmManager *, g_AnmManager);

void AnmManager::ExecuteScriptOnVmArray(Sprt *sprite, int count)
{
    for (int i = 0; i < count; i++)
    {
        if (sprite->scriptIndex > -1)
        {
            g_AnmManager->ExecuteScript(sprite);
        }
        sprite++;
    }
}

}; // Namespace th08
