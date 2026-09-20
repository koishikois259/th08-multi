#include "th_pch.h"

#ifdef TH08_MULTI

#include <stdlib.h>
#include <string.h>

#include "MultiLocalInput.hpp"
#include "Global.hpp"
#include "Supervisor.hpp"

namespace th08
{

namespace
{

const char *const LOCAL_CONFIG_PATH = ".\\th08_multi.ini";

void FormatGamepadGuid(const GUID &guid, char *text)
{
    wsprintfA(text,
              "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
              guid.Data1, guid.Data2, guid.Data3,
              guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
              guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

void AddMappedButton(u16 *buttons, const u8 *state, i16 index, u16 button)
{
    if (index >= 0 && index < 128 && (state[index] & 0x80) != 0)
        *buttons |= button;
}

} // namespace

MultiLocalInput g_MultiLocalInput;

MultiLocalInput::MultiLocalInput()
{
    memset(slots, 0, sizeof(slots));
    window = NULL;
    directInput = NULL;
    initialized = false;
}

bool MultiLocalInput::ResolveKeyboard(DeviceSlot *slot)
{
    UINT count = 0;
    RAWINPUTDEVICELIST *devices;
    UINT index;
    if (GetRawInputDeviceList(NULL, &count, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1 || count == 0)
        return false;
    devices = static_cast<RAWINPUTDEVICELIST *>(malloc(sizeof(RAWINPUTDEVICELIST) * count));
    if (devices == NULL)
        return false;
    if (GetRawInputDeviceList(devices, &count, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1)
    {
        free(devices);
        return false;
    }
    for (index = 0; index < count; ++index)
    {
        char name[512];
        UINT nameLength = sizeof(name);
        if (devices[index].dwType != RIM_TYPEKEYBOARD)
            continue;
        if (GetRawInputDeviceInfoA(devices[index].hDevice, RIDI_DEVICENAME,
                                   name, &nameLength) == (UINT)-1)
            continue;
        if (lstrcmpiA(name, slot->identifier) == 0)
        {
            slot->keyboardHandle = devices[index].hDevice;
            break;
        }
    }
    free(devices);
    return slot->keyboardHandle != NULL;
}

BOOL CALLBACK MultiLocalInput::SetGamepadAxisRange(
    const DIDEVICEOBJECTINSTANCEA *object, VOID *context)
{
    LPDIRECTINPUTDEVICE8A gamepad = static_cast<LPDIRECTINPUTDEVICE8A>(context);
    if ((object->dwType & 3) != 0)
    {
        DIPROPRANGE range;
        memset(&range, 0, sizeof(range));
        range.diph.dwSize = sizeof(range);
        range.diph.dwHeaderSize = sizeof(range.diph);
        range.diph.dwHow = DIPH_BYID;
        range.diph.dwObj = object->dwType;
        range.lMin = -1000;
        range.lMax = 1000;
        if (FAILED(gamepad->SetProperty(DIPROP_RANGE, &range.diph)))
            return DIENUM_STOP;
    }
    return DIENUM_CONTINUE;
}

BOOL CALLBACK MultiLocalInput::EnumerateGamepads(
    const DIDEVICEINSTANCEA *device, VOID *context)
{
    DeviceSlot *slot = static_cast<DeviceSlot *>(context);
    char guidText[40];
    FormatGamepadGuid(device->guidInstance, guidText);
    if (lstrcmpiA(slot->identifier, guidText) != 0)
        return DIENUM_CONTINUE;
    if (FAILED(g_MultiLocalInput.directInput->CreateDevice(
            device->guidInstance, &slot->gamepad, NULL)))
        return DIENUM_CONTINUE;
    return DIENUM_STOP;
}

bool MultiLocalInput::ResolveGamepad(DeviceSlot *slot)
{
    if (directInput == NULL ||
        FAILED(directInput->EnumDevices(DI8DEVCLASS_GAMECTRL,
                                       EnumerateGamepads, slot, DIEDFL_ATTACHEDONLY)) ||
        slot->gamepad == NULL)
        return false;
    if (FAILED(slot->gamepad->SetDataFormat(&c_dfDIJoystick2)) ||
        FAILED(slot->gamepad->SetCooperativeLevel(window,
                                                  DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
        return false;
    slot->gamepad->EnumObjects(SetGamepadAxisRange, slot->gamepad, DIDFT_AXIS);
    slot->gamepad->Acquire();
    return true;
}

bool MultiLocalInput::Initialize(HWND gameWindow)
{
    char type[16];
    bool needsGamepad = false;
    bool needsKeyboard = false;
    int index;
    if (initialized)
        return true;
    window = gameWindow;
    for (index = 0; index < 2; ++index)
    {
        char typeKey[16];
        char idKey[16];
        wsprintfA(typeKey, "p%d_type", index + 1);
        wsprintfA(idKey, "p%d_id", index + 1);
        GetPrivateProfileStringA("local", typeKey, "", type, sizeof(type), LOCAL_CONFIG_PATH);
        slots[index].keyboard = lstrcmpiA(type, "keyboard") == 0;
        if (!slots[index].keyboard && lstrcmpiA(type, "gamepad") != 0)
            goto invalid_device;
        GetPrivateProfileStringA("local", idKey, "", slots[index].identifier,
                                 sizeof(slots[index].identifier), LOCAL_CONFIG_PATH);
        if (slots[index].identifier[0] == '\0')
            goto invalid_device;
        needsKeyboard |= slots[index].keyboard;
        needsGamepad |= !slots[index].keyboard;
    }
    if (slots[0].keyboard == slots[1].keyboard &&
        lstrcmpiA(slots[0].identifier, slots[1].identifier) == 0)
        goto invalid_device;

    if (needsGamepad && FAILED(DirectInput8Create(
            GetModuleHandleA(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8A,
            reinterpret_cast<void **>(&directInput), NULL)))
        goto invalid_device;
    for (index = 0; index < 2; ++index)
    {
        if (slots[index].keyboard ? !ResolveKeyboard(&slots[index])
                                  : !ResolveGamepad(&slots[index]))
            goto invalid_device;
    }
    if (needsKeyboard)
    {
        RAWINPUTDEVICE keyboard;
        keyboard.usUsagePage = 1;
        keyboard.usUsage = 6;
        keyboard.dwFlags = 0;
        keyboard.hwndTarget = window;
        if (!RegisterRawInputDevices(&keyboard, 1, sizeof(keyboard)))
            goto invalid_device;
    }
    initialized = true;
    g_GameErrorContext.Log("multi local: two physical input devices ready\n");
    return true;

invalid_device:
    g_GameErrorContext.Log("multi local: configured device missing or invalid\n");
    MessageBoxA(window,
                "A selected input device is unavailable. Return to the launcher, refresh devices and select two different devices.",
                "Local play input error", MB_OK | MB_ICONERROR);
    Shutdown();
    return false;
}

bool MultiLocalInput::RebindWindow(HWND gameWindow)
{
    int index;
    bool hasKeyboard = false;
    RAWINPUTDEVICE keyboard;
    if (!initialized || gameWindow == NULL || gameWindow == window)
        return true;
    for (index = 0; index < 2; ++index)
    {
        hasKeyboard |= slots[index].keyboard;
        if (slots[index].gamepad != NULL)
        {
            slots[index].gamepad->Unacquire();
            if (FAILED(slots[index].gamepad->SetCooperativeLevel(
                    gameWindow, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
                return false;
            slots[index].gamepad->Acquire();
        }
    }
    if (hasKeyboard)
    {
        keyboard.usUsagePage = 1;
        keyboard.usUsage = 6;
        keyboard.dwFlags = 0;
        keyboard.hwndTarget = gameWindow;
        if (!RegisterRawInputDevices(&keyboard, 1, sizeof(keyboard)))
            return false;
    }
    window = gameWindow;
    OnFocusLost();
    return true;
}

void MultiLocalInput::Shutdown()
{
    int index;
    RAWINPUTDEVICE keyboard;
    if (initialized && (slots[0].keyboard || slots[1].keyboard))
    {
        keyboard.usUsagePage = 1;
        keyboard.usUsage = 6;
        keyboard.dwFlags = RIDEV_REMOVE;
        keyboard.hwndTarget = NULL;
        RegisterRawInputDevices(&keyboard, 1, sizeof(keyboard));
    }
    for (index = 0; index < 2; ++index)
    {
        if (slots[index].gamepad != NULL)
        {
            slots[index].gamepad->Unacquire();
            slots[index].gamepad->Release();
            slots[index].gamepad = NULL;
        }
    }
    if (directInput != NULL)
    {
        directInput->Release();
        directInput = NULL;
    }
    initialized = false;
    window = NULL;
}

void MultiLocalInput::OnRawInput(HRAWINPUT input)
{
    RAWINPUT raw;
    UINT size = sizeof(raw);
    int index;
    u16 scanCode;
    if (!initialized || GetRawInputData(input, RID_INPUT, &raw, &size,
                                        sizeof(RAWINPUTHEADER)) == (UINT)-1 ||
        raw.header.dwType != RIM_TYPEKEYBOARD)
        return;
    scanCode = static_cast<u16>(raw.data.keyboard.MakeCode & 0x7f);
    if (raw.data.keyboard.Flags & RI_KEY_E0)
        scanCode |= 0x80;
    if (scanCode == 0)
        return;
    for (index = 0; index < 2; ++index)
    {
        if (slots[index].keyboard && slots[index].keyboardHandle == raw.header.hDevice)
        {
            slots[index].keys[scanCode] =
                (raw.data.keyboard.Flags & RI_KEY_BREAK) != 0 ? 0 : 0x80;
            return;
        }
    }
}

void MultiLocalInput::OnFocusLost()
{
    memset(slots[0].keys, 0, sizeof(slots[0].keys));
    memset(slots[1].keys, 0, sizeof(slots[1].keys));
}

u16 MultiLocalInput::GetKeyboardInput(const DeviceSlot &slot) const
{
    const u8 *keys = slot.keys;
    u16 buttons = 0;
#define LOCAL_KEY(button, key) if (keys[key] != 0) buttons |= button
    LOCAL_KEY(TH_BUTTON_UP, DIK_UP);
    LOCAL_KEY(TH_BUTTON_DOWN, DIK_DOWN);
    LOCAL_KEY(TH_BUTTON_LEFT, DIK_LEFT);
    LOCAL_KEY(TH_BUTTON_RIGHT, DIK_RIGHT);
    LOCAL_KEY(TH_BUTTON_UP, DIK_NUMPAD8);
    LOCAL_KEY(TH_BUTTON_DOWN, DIK_NUMPAD2);
    LOCAL_KEY(TH_BUTTON_LEFT, DIK_NUMPAD4);
    LOCAL_KEY(TH_BUTTON_RIGHT, DIK_NUMPAD6);
    LOCAL_KEY(TH_BUTTON_UP_LEFT, DIK_NUMPAD7);
    LOCAL_KEY(TH_BUTTON_UP_RIGHT, DIK_NUMPAD9);
    LOCAL_KEY(TH_BUTTON_DOWN_LEFT, DIK_NUMPAD1);
    LOCAL_KEY(TH_BUTTON_DOWN_RIGHT, DIK_NUMPAD3);
    LOCAL_KEY(TH_BUTTON_HOME, DIK_HOME);
    LOCAL_KEY(TH_BUTTON_HOME, DIK_P);
    LOCAL_KEY(TH_BUTTON_D, DIK_D);
    LOCAL_KEY(TH_BUTTON_SHOOT, DIK_Z);
    LOCAL_KEY(TH_BUTTON_BOMB, DIK_X);
    LOCAL_KEY(TH_BUTTON_FOCUS, DIK_LSHIFT);
    LOCAL_KEY(TH_BUTTON_FOCUS, DIK_RSHIFT);
    LOCAL_KEY(TH_BUTTON_MENU, DIK_ESCAPE);
    LOCAL_KEY(TH_BUTTON_SKIP, DIK_LCONTROL);
    LOCAL_KEY(TH_BUTTON_SKIP, DIK_RCONTROL);
    LOCAL_KEY(TH_BUTTON_Q, DIK_Q);
    LOCAL_KEY(TH_BUTTON_S, DIK_S);
    LOCAL_KEY(TH_BUTTON_RESET, DIK_R);
    LOCAL_KEY(TH_BUTTON_ENTER, DIK_RETURN);
#undef LOCAL_KEY
    return buttons;
}

u16 MultiLocalInput::GetGamepadInput(DeviceSlot *slot)
{
    DIJOYSTATE2 state;
    u16 buttons = 0;
    const ControllerMapping &mapping = g_Supervisor.cfg.controllerMapping;
    if (slot->gamepad == NULL)
        return 0;
    if (FAILED(slot->gamepad->Poll()))
        slot->gamepad->Acquire();
    memset(&state, 0, sizeof(state));
    if (FAILED(slot->gamepad->GetDeviceState(sizeof(state), &state)))
        return 0;
    AddMappedButton(&buttons, state.rgbButtons, mapping.shotButton, TH_BUTTON_SHOOT);
    AddMappedButton(&buttons, state.rgbButtons, mapping.bombButton, TH_BUTTON_BOMB);
    AddMappedButton(&buttons, state.rgbButtons, mapping.focusButton, TH_BUTTON_FOCUS);
    AddMappedButton(&buttons, state.rgbButtons, mapping.menuButton, TH_BUTTON_MENU);
    AddMappedButton(&buttons, state.rgbButtons, mapping.upButton, TH_BUTTON_UP);
    AddMappedButton(&buttons, state.rgbButtons, mapping.downButton, TH_BUTTON_DOWN);
    AddMappedButton(&buttons, state.rgbButtons, mapping.leftButton, TH_BUTTON_LEFT);
    AddMappedButton(&buttons, state.rgbButtons, mapping.rightButton, TH_BUTTON_RIGHT);
    AddMappedButton(&buttons, state.rgbButtons, mapping.skipButton, TH_BUTTON_SKIP);
    if (g_Supervisor.IsShotSlowEnabled() &&
        mapping.shotButton >= 0 && mapping.shotButton < 128 &&
        (state.rgbButtons[mapping.shotButton] & 0x80) != 0)
    {
        if (slot->shotSlowFrames < 20)
            ++slot->shotSlowFrames;
        if (slot->shotSlowFrames >= 10)
            buttons |= TH_BUTTON_FOCUS;
    }
    else
        slot->shotSlowFrames = 0;
    if (state.lX > g_Supervisor.cfg.padXAxis) buttons |= TH_BUTTON_RIGHT;
    if (state.lX < -g_Supervisor.cfg.padXAxis) buttons |= TH_BUTTON_LEFT;
    if (state.lY > g_Supervisor.cfg.padYAxis) buttons |= TH_BUTTON_DOWN;
    if (state.lY < -g_Supervisor.cfg.padYAxis) buttons |= TH_BUTTON_UP;
    if ((state.rgdwPOV[0] & 0xffff) != 0xffff)
    {
        DWORD angle = state.rgdwPOV[0];
        if (angle >= 31500 || angle <= 4500) buttons |= TH_BUTTON_UP;
        if (angle >= 4500 && angle <= 13500) buttons |= TH_BUTTON_RIGHT;
        if (angle >= 13500 && angle <= 22500) buttons |= TH_BUTTON_DOWN;
        if (angle >= 22500 && angle <= 31500) buttons |= TH_BUTTON_LEFT;
    }
    return buttons;
}

u16 MultiLocalInput::GetInput(u8 slot)
{
    if (!initialized || slot >= 2)
        return 0;
    return slots[slot].keyboard ? GetKeyboardInput(slots[slot])
                                : GetGamepadInput(&slots[slot]);
}

} // namespace th08

#endif
