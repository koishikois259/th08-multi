#pragma once

#include <windows.h>

namespace th08
{

enum MultiLocalKeyboardAction
{
    LOCAL_KEY_UP,
    LOCAL_KEY_DOWN,
    LOCAL_KEY_LEFT,
    LOCAL_KEY_RIGHT,
    LOCAL_KEY_FOCUS,
    LOCAL_KEY_SHOOT,
    LOCAL_KEY_BOMB,
    LOCAL_KEY_COUNT,
};

struct MultiLocalKeyboardConfig
{
    unsigned int p1Keys[LOCAL_KEY_COUNT];
    unsigned int p2Keys[LOCAL_KEY_COUNT];
};

inline const unsigned int *MultiLocalKeys(const MultiLocalKeyboardConfig &config, unsigned int slot)
{
    return slot == 0 ? config.p1Keys : config.p2Keys;
}

inline unsigned int *MultiLocalKeys(MultiLocalKeyboardConfig &config, unsigned int slot)
{
    return slot == 0 ? config.p1Keys : config.p2Keys;
}

inline const char *MultiLocalKeySetting(unsigned int slot, int action)
{
    static const char *const names[2][LOCAL_KEY_COUNT] = {
        {"p1_up", "p1_down", "p1_left", "p1_right", "p1_focus", "p1_shoot", "p1_bomb"},
        {"p2_up", "p2_down", "p2_left", "p2_right", "p2_focus", "p2_shoot", "p2_bomb"}};
    return names[slot][action];
}

inline unsigned int CanonicalMultiLocalKey(unsigned int key)
{
    // Generic Shift reads both physical Shift keys, so they must be treated
    // as one binding for conflict detection and persistence.
    return key == VK_LSHIFT || key == VK_RSHIFT ? VK_SHIFT : key;
}

inline void DefaultMultiLocalKeyboardConfig(MultiLocalKeyboardConfig *config)
{
    const unsigned int defaults[LOCAL_KEY_COUNT] = {'I', 'K', 'J', 'L', 'D', 'F', 'G'};
    const unsigned int p1Defaults[LOCAL_KEY_COUNT] = {
        VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_SHIFT, 'Z', 'X'};
    for (int i = 0; i < LOCAL_KEY_COUNT; ++i)
    {
        config->p1Keys[i] = p1Defaults[i];
        config->p2Keys[i] = defaults[i];
    }
}

inline bool IsReservedMultiLocalKey(unsigned int key)
{
    // Shared menu/skip and system controls remain reserved. Gameplay keys
    // belong only to their configured action, never to a fixed P1 alias.
    return key == VK_ESCAPE ||
           key == VK_RETURN || key == VK_CONTROL || key == VK_LCONTROL ||
           key == VK_RCONTROL || key == VK_MENU || key == VK_LMENU ||
           key == VK_RMENU || key == VK_LWIN || key == VK_RWIN ||
           key == VK_TAB || key == VK_F10;
}

inline bool IsAssignableMultiLocalKey(unsigned int key)
{
    if (IsReservedMultiLocalKey(key))
        return false;
    return (key >= '0' && key <= '9') || (key >= 'A' && key <= 'Z') ||
           (key >= VK_NUMPAD0 && key <= VK_DIVIDE) ||
           (key >= VK_F1 && key <= VK_F12) || key == VK_SPACE ||
           key == VK_BACK || key == VK_PRIOR || key == VK_NEXT ||
           key == VK_END || key == VK_HOME || key == VK_INSERT ||
           key == VK_DELETE || key == VK_UP || key == VK_DOWN || key == VK_LEFT ||
           key == VK_RIGHT || key == VK_SHIFT || key == VK_LSHIFT || key == VK_RSHIFT ||
           (key >= VK_OEM_1 && key <= VK_OEM_3) ||
           (key >= VK_OEM_4 && key <= VK_OEM_8) || key == VK_OEM_102;
}

inline bool ValidateMultiLocalKeyboardConfig(const MultiLocalKeyboardConfig &config)
{
    for (int i = 0; i < 2 * LOCAL_KEY_COUNT; ++i)
    {
        unsigned int key = MultiLocalKeys(config, i / LOCAL_KEY_COUNT)[i % LOCAL_KEY_COUNT];
        if (!IsAssignableMultiLocalKey(key))
            return false;
        for (int j = 0; j < i; ++j)
            if (CanonicalMultiLocalKey(key) == CanonicalMultiLocalKey(
                    MultiLocalKeys(config, j / LOCAL_KEY_COUNT)[j % LOCAL_KEY_COUNT]))
                return false;
    }
    return true;
}

inline bool LoadMultiLocalKeyboardConfig(const char *path, MultiLocalKeyboardConfig *config)
{
    DefaultMultiLocalKeyboardConfig(config);
    for (unsigned int slot = 0; slot < 2; ++slot)
        for (int i = 0; i < LOCAL_KEY_COUNT; ++i)
        {
            unsigned int *keys = MultiLocalKeys(*config, slot);
            keys[i] = CanonicalMultiLocalKey(GetPrivateProfileIntA(
                "local_keyboard", MultiLocalKeySetting(slot, i), keys[i], path));
        }
    if (ValidateMultiLocalKeyboardConfig(*config))
        return true;
    // A manually edited or partial invalid file must never bind both players
    // to the same key. Fall back as a complete two-player set, without writing.
    DefaultMultiLocalKeyboardConfig(config);
    return false;
}

inline bool SaveMultiLocalKeyboardConfig(const char *path, const MultiLocalKeyboardConfig &config)
{
    char section[512];
    char *next = section;
    if (!ValidateMultiLocalKeyboardConfig(config))
        return false;
    for (unsigned int slot = 0; slot < 2; ++slot)
        for (int i = 0; i < LOCAL_KEY_COUNT; ++i)
            next += wsprintfA(next, "%s=%u", MultiLocalKeySetting(slot, i),
                              CanonicalMultiLocalKey(MultiLocalKeys(config, slot)[i])) + 1;
    *next = '\0';
    // One section write avoids storing a half-edited two-player mapping.
    return WritePrivateProfileSectionA("local_keyboard", section, path) != FALSE;
}

inline bool IsSingleKeyboardLocalConfig(const char *path)
{
    char mode[16];
    GetPrivateProfileStringA("local", "input_mode", "double", mode, sizeof(mode), path);
    return lstrcmpiA(mode, "single") == 0;
}

inline unsigned short SingleKeyboardLocalButtons(
    unsigned int slot, const unsigned char *state, const MultiLocalKeyboardConfig &config)
{
    // TH08 action bits; no retail headers or layouts are changed by this helper.
    const unsigned short buttons[LOCAL_KEY_COUNT] = {16, 32, 64, 128, 4, 1, 2};
    unsigned short result = 0;
    if (slot > 1 || !ValidateMultiLocalKeyboardConfig(config))
        return 0;
    for (int i = 0; i < LOCAL_KEY_COUNT; ++i)
    {
        unsigned int key = CanonicalMultiLocalKey(MultiLocalKeys(config, slot)[i]);
        if ((state[key] & 0x80) != 0 ||
            (key == VK_SHIFT && ((state[VK_LSHIFT] | state[VK_RSHIFT]) & 0x80) != 0))
            result |= buttons[i];
    }
    if (slot == 0)
    {
        if (state[VK_ESCAPE] & 0x80)
            result |= 8;
        if (state[VK_RETURN] & 0x80)
            result |= 4096;
        if ((state[VK_CONTROL] | state[VK_LCONTROL] | state[VK_RCONTROL]) & 0x80)
            result |= 256;
    }
    return result;
}

} // namespace th08
