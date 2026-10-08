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
    const unsigned int p1Defaults[] = {VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_SHIFT, 'Z', 'X'};
    const unsigned short masks[] = {16, 32, 64, 128, 4, 1, 2};
    for (int i = 0; i < LOCAL_KEY_COUNT; ++i)
    {
        Expect(config.p2Keys[i] == defaults[i], "exact default key");
        Expect(config.p1Keys[i] == p1Defaults[i], "P1 defaults unchanged");
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

    const unsigned int customP1[] = {'W', 'S', 'A', 'E', 'Q', 'R', 'T'};
    for (int i = 0; i < LOCAL_KEY_COUNT; ++i)
        config.p1Keys[i] = customP1[i];
    Expect(ValidateMultiLocalKeyboardConfig(config), "complete custom P1 accepted");
    for (int i = 0; i < LOCAL_KEY_COUNT; ++i)
    {
        memset(state, 0, sizeof(state));
        state[customP1[i]] = 0x80;
        Expect(SingleKeyboardLocalButtons(0, state, config) == masks[i], "custom P1 action routed");
        Expect(SingleKeyboardLocalButtons(1, state, config) == 0, "custom P1 does not control P2");
        memset(state, 0, sizeof(state));
        state[p1Defaults[i]] = state[VK_LSHIFT] = state[VK_RSHIFT] = 0x80;
        Expect(SingleKeyboardLocalButtons(0, state, config) == 0, "old P1 keys no longer act as aliases");
    }
    MultiLocalKeyboardConfig moved = config;
    for (int i = 0; i < LOCAL_KEY_COUNT; ++i)
        moved.p2Keys[i] = p1Defaults[i];
    Expect(ValidateMultiLocalKeyboardConfig(moved), "P2 can use released P1 default keys");
    memset(state, 0, sizeof(state));
    state[VK_RSHIFT] = 0x80;
    Expect(SingleKeyboardLocalButtons(1, state, moved) == 4 &&
           SingleKeyboardLocalButtons(0, state, moved) == 0,
           "Shift aliases apply only to configured player/action");
    moved.p1Keys[LOCAL_KEY_UP] = VK_LSHIFT;
    Expect(!ValidateMultiLocalKeyboardConfig(moved), "generic and side Shift collision rejected");
    moved = config;
    moved.p1Keys[LOCAL_KEY_UP] = moved.p2Keys[LOCAL_KEY_SHOOT];
    Expect(!ValidateMultiLocalKeyboardConfig(moved), "cross-player custom conflict rejected");
    moved = config;
    moved.p1Keys[LOCAL_KEY_UP] = moved.p1Keys[LOCAL_KEY_DOWN];
    Expect(!ValidateMultiLocalKeyboardConfig(moved), "duplicate P1 action key rejected");

    char tempDirectory[MAX_PATH], path[MAX_PATH];
    GetTempPathA(sizeof(tempDirectory), tempDirectory);
    Expect(GetTempFileNameA(tempDirectory, "t8k", 0, path) != 0, "temporary config created");
    WritePrivateProfileStringA("network", "mode", "guest", path);
    WritePrivateProfileStringA("network", "host", "192.0.2.10", path);
    WritePrivateProfileStringA("local", "p1_id", "test-device", path);
    WritePrivateProfileStringA("local", "input_mode", "double", path);
    Expect(!IsSingleKeyboardLocalConfig(path), "old/double mode remains double");
    WritePrivateProfileStringA("local_keyboard", "p2_shoot", "72", path);
    MultiLocalKeyboardConfig legacy;
    Expect(LoadMultiLocalKeyboardConfig(path, &legacy) && legacy.p2Keys[LOCAL_KEY_SHOOT] == 'H',
           "legacy P2-only mapping retained");
    for (int i = 0; i < LOCAL_KEY_COUNT; ++i)
        Expect(legacy.p1Keys[i] == p1Defaults[i], "missing P1 keys load old defaults");
    Expect(SaveMultiLocalKeyboardConfig(path, config), "custom config saved");
    MultiLocalKeyboardConfig loaded;
    Expect(LoadMultiLocalKeyboardConfig(path, &loaded) && loaded.p2Keys[LOCAL_KEY_SHOOT] == 'H', "custom config roundtrip");
    for (int i = 0; i < LOCAL_KEY_COUNT; ++i)
        Expect(loaded.p1Keys[i] == customP1[i], "P1 custom config roundtrip");
    char value[64];
    GetPrivateProfileStringA("network", "mode", "", value, sizeof(value), path);
    Expect(strcmp(value, "guest") == 0, "online mode untouched by key save");
    GetPrivateProfileStringA("network", "host", "", value, sizeof(value), path);
    Expect(strcmp(value, "192.0.2.10") == 0, "online address untouched");
    GetPrivateProfileStringA("local", "p1_id", "", value, sizeof(value), path);
    Expect(strcmp(value, "test-device") == 0, "double device choice untouched");
    Expect(!IsSingleKeyboardLocalConfig(path), "key save does not switch double mode");
    WritePrivateProfileStringA("local", "input_mode", "single", path);
    Expect(IsSingleKeyboardLocalConfig(path), "single mode loaded");
    WritePrivateProfileStringA("local_keyboard", "p2_up", "87", path);
    Expect(!LoadMultiLocalKeyboardConfig(path, &loaded), "manual P1 conflict rejected");
    for (int i = 0; i < LOCAL_KEY_COUNT; ++i)
    {
        Expect(loaded.p2Keys[i] == defaults[i], "invalid file resets complete mapping");
        Expect(loaded.p1Keys[i] == p1Defaults[i], "invalid file resets P1 as well");
    }
    config.p2Keys[0] = 'W';
    Expect(!SaveMultiLocalKeyboardConfig(path, config), "invalid config cannot be saved");
    DeleteFileA(path);
    printf("Local keyboard tests: %s\n", failures == 0 ? "PASS" : "FAIL");
    return failures != 0;
}
