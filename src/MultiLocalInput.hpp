#pragma once

#ifdef TH08_MULTI

#include <windows.h>
#include <dinput.h>

#include "inttypes.hpp"

namespace th08
{

// Local play deliberately keeps device state outside the retail Supervisor ABI.
// Raw Input is required here: DirectInput's system keyboard merges every
// physical keyboard into one logical device.
class MultiLocalInput
{
  public:
    MultiLocalInput();
    bool Initialize(HWND window);
    bool RebindWindow(HWND window);
    void Shutdown();
    void OnRawInput(HRAWINPUT input);
    void OnFocusLost();
    u16 GetInput(u8 slot);

  private:
    struct DeviceSlot
    {
        bool keyboard;
        char identifier[512];
        HANDLE keyboardHandle;
        LPDIRECTINPUTDEVICE8A gamepad;
        u8 keys[256];
        u8 shotSlowFrames;
    };

    static BOOL CALLBACK EnumerateGamepads(const DIDEVICEINSTANCEA *device, VOID *context);
    static BOOL CALLBACK SetGamepadAxisRange(const DIDEVICEOBJECTINSTANCEA *object, VOID *context);
    bool ResolveKeyboard(DeviceSlot *slot);
    bool ResolveGamepad(DeviceSlot *slot);
    u16 GetKeyboardInput(const DeviceSlot &slot) const;
    u16 GetGamepadInput(DeviceSlot *slot);

    HWND window;
    LPDIRECTINPUT8A directInput;
    DeviceSlot slots[2];
    bool initialized;
};

extern MultiLocalInput g_MultiLocalInput;

} // namespace th08

#endif
