#include "th_pch.h"
#include "MultiKeyboardInput.hpp"
#include "Supervisor.hpp"

namespace th08
{
namespace
{

MultiKeyboardRecovery keyboardRecovery;
MultiKeyboardReadSource lastSource = MULTI_KEYBOARD_BACKGROUND;

struct WindowsKeyboardBackend
{
    HRESULT readResult;
    HRESULT acquireResult;

    WindowsKeyboardBackend() : readResult(DIERR_NOTACQUIRED), acquireResult(S_OK) {}
    bool IsForeground()
    {
        HWND window = g_Supervisor.hwndGameWindow;
        return window != NULL && GetForegroundWindow() == window && !IsIconic(window);
    }
    bool HasDirectInput() { return g_Supervisor.keyboard != NULL; }
    bool Acquire()
    {
        acquireResult = g_Supervisor.keyboard->Acquire();
        return SUCCEEDED(acquireResult);
    }
    bool ReadDirectInput(u8 *state)
    {
        readResult = g_Supervisor.keyboard->GetDeviceState(256, state);
        return SUCCEEDED(readResult);
    }
    void ReadFallback(u8 *state)
    {
        size_t count;
        const MultiKeyboardBinding *bindings = GetMultiKeyboardBindings(&count);
        for (size_t index = 0; index < count; ++index)
        {
            // Use only the current-down bit, never the unreliable press-history bit.
            if ((GetAsyncKeyState(bindings[index].virtualKey) & 0x8000) != 0)
                state[bindings[index].scanCode] = 0x80;
        }
    }
};

} // namespace

u16 ReadMultiKeyboardButtons()
{
    u8 state[256];
    WindowsKeyboardBackend backend;
    MultiKeyboardReadSource source = keyboardRecovery.Read(backend, state);
    if (source != lastSource)
    {
        if (source == MULTI_KEYBOARD_FALLBACK)
            g_GameErrorContext.Log("multi keyboard: foreground fallback read=%08x acquire=%08x\n",
                                   static_cast<u32>(backend.readResult),
                                   static_cast<u32>(backend.acquireResult));
        else if (source == MULTI_KEYBOARD_DIRECTINPUT)
            g_GameErrorContext.Log("multi keyboard: DirectInput ready\n");
        else
            g_GameErrorContext.Log("multi keyboard: background input suppressed\n");
        lastSource = source;
    }
    return MultiKeyboardStateToButtons(state);
}

void ResetMultiKeyboardInput()
{
    keyboardRecovery.OnFocusChanged();
    lastSource = MULTI_KEYBOARD_BACKGROUND;
}

void NotifyMultiKeyboardFocusChange()
{
    // WM_ACTIVATEAPP can arrive before DirectInput exists. Defer acquisition
    // until the next foreground input read, rather than touching a null device.
    keyboardRecovery.OnFocusChanged();
}

} // namespace th08
