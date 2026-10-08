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
    unsigned int p2Keys[LOCAL_KEY_COUNT];
};

inline const char *MultiLocalKeySetting(int action)
{
    static const char *const names[LOCAL_KEY_COUNT] = {
        "p2_up", "p2_down", "p2_left", "p2_right",
        "p2_focus", "p2_shoot", "p2_bomb"};
    return names[action];
}

inline void DefaultMultiLocalKeyboardConfig(MultiLocalKeyboardConfig *config)
{
    const unsigned int defaults[LOCAL_KEY_COUNT] = {'I', 'K', 'J', 'L', 'D', 'F', 'G'};
    for (int i = 0; i < LOCAL_KEY_COUNT; ++i)
        config->p2Keys[i] = defaults[i];
}

inline bool IsReservedMultiLocalKey(unsigned int key)
{
    // Single-keyboard P1 has only the seven advertised gameplay bindings,
    // plus shared menu/skip controls. In particular D belongs to P2 here;
    // retail debug/cheat and numpad aliases are not merged into this mode.
    return key == VK_UP || key == VK_DOWN || key == VK_LEFT || key == VK_RIGHT ||
           key == 'Z' || key == 'X' || key == VK_SHIFT ||
           key == VK_LSHIFT || key == VK_RSHIFT || key == VK_ESCAPE ||
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
           key == VK_DELETE || (key >= VK_OEM_1 && key <= VK_OEM_3) ||
           (key >= VK_OEM_4 && key <= VK_OEM_8) || key == VK_OEM_102;
}

inline bool ValidateMultiLocalKeyboardConfig(const MultiLocalKeyboardConfig &config)
{
    for (int i = 0; i < LOCAL_KEY_COUNT; ++i)
    {
        if (!IsAssignableMultiLocalKey(config.p2Keys[i]))
            return false;
        for (int j = 0; j < i; ++j)
            if (config.p2Keys[i] == config.p2Keys[j])
                return false;
    }
    return true;
}

inline bool LoadMultiLocalKeyboardConfig(const char *path, MultiLocalKeyboardConfig *config)
{
    DefaultMultiLocalKeyboardConfig(config);
    for (int i = 0; i < LOCAL_KEY_COUNT; ++i)
        config->p2Keys[i] = GetPrivateProfileIntA(
            "local_keyboard", MultiLocalKeySetting(i), config->p2Keys[i], path);
    if (ValidateMultiLocalKeyboardConfig(*config))
        return true;
    // A manually edited or partial invalid file must never bind both players
    // to the same key. Fall back as a complete set, without modifying the file.
    DefaultMultiLocalKeyboardConfig(config);
    return false;
}

inline bool SaveMultiLocalKeyboardConfig(const char *path, const MultiLocalKeyboardConfig &config)
{
    char section[512];
    char *next = section;
    if (!ValidateMultiLocalKeyboardConfig(config))
        return false;
    for (int i = 0; i < LOCAL_KEY_COUNT; ++i)
        next += wsprintfA(next, "%s=%u", MultiLocalKeySetting(i), config.p2Keys[i]) + 1;
    *next = '\0';
    // One section write avoids storing a half-edited seven-key mapping.
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
    const unsigned int p1Keys[LOCAL_KEY_COUNT] = {
        VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_SHIFT, 'Z', 'X'};
    // TH08 action bits; no retail headers or layouts are changed by this helper.
    const unsigned short buttons[LOCAL_KEY_COUNT] = {16, 32, 64, 128, 4, 1, 2};
    unsigned short result = 0;
    if (slot > 1 || !ValidateMultiLocalKeyboardConfig(config))
        return 0;
    for (int i = 0; i < LOCAL_KEY_COUNT; ++i)
    {
        unsigned int key = slot == 0 ? p1Keys[i] : config.p2Keys[i];
        if ((state[key] & 0x80) != 0)
            result |= buttons[i];
    }
    if (slot == 0)
    {
        if ((state[VK_LSHIFT] | state[VK_RSHIFT]) & 0x80)
            result |= 4;
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
