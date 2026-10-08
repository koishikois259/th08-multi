#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "MultiLocalKeyboardConfig.hpp"

using namespace th08;
static int failures = 0;
static void Expect(bool condition, const char *name)
{
    if (!condition) { ++failures; printf("FAIL: %s\n", name); }
}

int main()
{
    MultiLocalKeyboardConfig config;
    unsigned char state[256];
    DefaultMultiLocalKeyboardConfig(&config);
    Expect(ValidateMultiLocalKeyboardConfig(config), "defaults valid");
    const unsigned int defaults[] = {'I', 'K', 'J', 'L', 'D', 'F', 'G'};
    const unsigned short masks[] = {16, 32, 64, 128, 4, 1, 2};
    for (int i = 0; i < LOCAL_KEY_COUNT; ++i)
    {
        Expect(config.p2Keys[i] == defaults[i], "exact default key");
        memset(state, 0, sizeof(state));
        state[defaults[i]] = 0x80;
        Expect(SingleKeyboardLocalButtons(1, state, config) == masks[i], "P2 action routed");
        Expect(SingleKeyboardLocalButtons(0, state, config) == 0, "P2 key cannot control P1");
    }
    memset(state, 0, sizeof(state));
    state[VK_UP] = state['Z'] = state[VK_LSHIFT] = 0x80;
    state['I'] = state['F'] = state['D'] = 0x80;
    Expect(SingleKeyboardLocalButtons(0, state, config) == 21, "simultaneous P1 keys");
    Expect(SingleKeyboardLocalButtons(1, state, config) == 21, "simultaneous P2 keys");
    memset(state, 0, sizeof(state));
    state[VK_ESCAPE] = state[VK_RETURN] = state[VK_CONTROL] = 0x80;
    Expect(SingleKeyboardLocalButtons(0, state, config) == (8 | 4096 | 256), "shared menu keys owned by P1");
    Expect(SingleKeyboardLocalButtons(1, state, config) == 0, "menu keys not duplicated");
    Expect(SingleKeyboardLocalButtons(2, state, config) == 0, "invalid player slot");
    const unsigned int reserved[] = {VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, 'Z', 'X',
        VK_SHIFT, VK_LSHIFT, VK_RSHIFT, VK_ESCAPE, VK_RETURN, VK_CONTROL, VK_MENU,
        VK_LWIN, VK_RWIN, VK_TAB, VK_F10, 0, 256, 0xffffffffu};
    for (int i = 0; i < static_cast<int>(sizeof(reserved) / sizeof(reserved[0])); ++i)
    {
        DefaultMultiLocalKeyboardConfig(&config);
        config.p2Keys[0] = reserved[i];
        Expect(!ValidateMultiLocalKeyboardConfig(config), "reserved/out-of-range keys rejected");
        Expect(SingleKeyboardLocalButtons(1, state, config) == 0, "bad config never indexes key state");
    }
    DefaultMultiLocalKeyboardConfig(&config);
    config.p2Keys[0] = config.p2Keys[1];
    Expect(!ValidateMultiLocalKeyboardConfig(config), "duplicate rejected");
    DefaultMultiLocalKeyboardConfig(&config);
    config.p2Keys[LOCAL_KEY_SHOOT] = 'H';
    memset(state, 0, sizeof(state));
    state['H'] = 0x80;
    Expect(SingleKeyboardLocalButtons(1, state, config) == 1, "custom key applied");
    state['H'] = 1;
    Expect(SingleKeyboardLocalButtons(1, state, config) == 0, "press-history bit ignored");

    char tempDirectory[MAX_PATH], path[MAX_PATH];
    GetTempPathA(sizeof(tempDirectory), tempDirectory);
    Expect(GetTempFileNameA(tempDirectory, "t8k", 0, path) != 0, "temporary config created");
    WritePrivateProfileStringA("network", "mode", "guest", path);
    WritePrivateProfileStringA("network", "host", "192.0.2.10", path);
    WritePrivateProfileStringA("local", "p1_id", "test-device", path);
    WritePrivateProfileStringA("local", "input_mode", "double", path);
    Expect(!IsSingleKeyboardLocalConfig(path), "old/double mode remains double");
    Expect(SaveMultiLocalKeyboardConfig(path, config), "custom config saved");
    MultiLocalKeyboardConfig loaded;
    Expect(LoadMultiLocalKeyboardConfig(path, &loaded) && loaded.p2Keys[LOCAL_KEY_SHOOT] == 'H', "custom config roundtrip");
    char value[64];
    GetPrivateProfileStringA("network", "mode", "", value, sizeof(value), path);
    Expect(strcmp(value, "guest") == 0, "online mode untouched by key save");
    GetPrivateProfileStringA("network", "host", "", value, sizeof(value), path);
    Expect(strcmp(value, "192.0.2.10") == 0, "online address untouched");
    GetPrivateProfileStringA("local", "p1_id", "", value, sizeof(value), path);
    Expect(strcmp(value, "test-device") == 0, "double device choice untouched");
    WritePrivateProfileStringA("local", "input_mode", "single", path);
    Expect(IsSingleKeyboardLocalConfig(path), "single mode loaded");
    WritePrivateProfileStringA("local_keyboard", "p2_up", "90", path);
    Expect(!LoadMultiLocalKeyboardConfig(path, &loaded), "manual P1 conflict rejected");
    for (int i = 0; i < LOCAL_KEY_COUNT; ++i)
        Expect(loaded.p2Keys[i] == defaults[i], "invalid file resets complete mapping");
    config.p2Keys[0] = 'Z';
    Expect(!SaveMultiLocalKeyboardConfig(path, config), "invalid config cannot be saved");
    DeleteFileA(path);
    printf("Local keyboard tests: %s\n", failures == 0 ? "PASS" : "FAIL");
    return failures != 0;
}
