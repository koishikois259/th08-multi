#pragma once

#include "Global.hpp"
#include <dinput.h>
#include <string.h>

namespace th08
{

struct MultiKeyboardBinding
{
    int virtualKey;
    u8 scanCode;
    u16 buttons;
};

inline const MultiKeyboardBinding *GetMultiKeyboardBindings(size_t *count)
{
    static const MultiKeyboardBinding bindings[] = {
        { VK_UP, DIK_UP, TH_BUTTON_UP }, { VK_DOWN, DIK_DOWN, TH_BUTTON_DOWN },
        { VK_LEFT, DIK_LEFT, TH_BUTTON_LEFT }, { VK_RIGHT, DIK_RIGHT, TH_BUTTON_RIGHT },
        { VK_NUMPAD8, DIK_NUMPAD8, TH_BUTTON_UP }, { VK_NUMPAD2, DIK_NUMPAD2, TH_BUTTON_DOWN },
        { VK_NUMPAD4, DIK_NUMPAD4, TH_BUTTON_LEFT }, { VK_NUMPAD6, DIK_NUMPAD6, TH_BUTTON_RIGHT },
        { VK_NUMPAD7, DIK_NUMPAD7, TH_BUTTON_UP_LEFT },
        { VK_NUMPAD9, DIK_NUMPAD9, TH_BUTTON_UP_RIGHT },
        { VK_NUMPAD1, DIK_NUMPAD1, TH_BUTTON_DOWN_LEFT },
        { VK_NUMPAD3, DIK_NUMPAD3, TH_BUTTON_DOWN_RIGHT },
        { VK_HOME, DIK_HOME, TH_BUTTON_HOME }, { 'P', DIK_P, TH_BUTTON_HOME },
        { 'D', DIK_D, TH_BUTTON_D }, { 'Z', DIK_Z, TH_BUTTON_SHOOT },
        { 'X', DIK_X, TH_BUTTON_BOMB },
        { VK_LSHIFT, DIK_LSHIFT, TH_BUTTON_FOCUS },
        { VK_RSHIFT, DIK_RSHIFT, TH_BUTTON_FOCUS },
        { VK_ESCAPE, DIK_ESCAPE, TH_BUTTON_MENU },
        { VK_LCONTROL, DIK_LCONTROL, TH_BUTTON_SKIP },
        { VK_RCONTROL, DIK_RCONTROL, TH_BUTTON_SKIP },
        { 'Q', DIK_Q, TH_BUTTON_Q }, { 'S', DIK_S, TH_BUTTON_S },
        { 'R', DIK_R, TH_BUTTON_RESET }, { VK_RETURN, DIK_RETURN, TH_BUTTON_ENTER },
    };
    *count = sizeof(bindings) / sizeof(bindings[0]);
    return bindings;
}

inline u16 MultiKeyboardStateToButtons(const u8 *state)
{
    size_t count;
    const MultiKeyboardBinding *bindings = GetMultiKeyboardBindings(&count);
    u16 buttons = 0;
    for (size_t index = 0; index < count; ++index)
        if ((state[bindings[index].scanCode] & 0x80) != 0)
            buttons |= bindings[index].buttons;
    return buttons;
}

enum MultiKeyboardReadSource
{
    MULTI_KEYBOARD_BACKGROUND,
    MULTI_KEYBOARD_DIRECTINPUT,
    MULTI_KEYBOARD_FALLBACK,
};

// The same bounded recovery policy is exercised with a fake backend in tests.
class MultiKeyboardRecovery
{
  public:
    MultiKeyboardRecovery() : acquirePending(true) {}
    void OnFocusChanged() { acquirePending = true; }

    template <class Backend>
    MultiKeyboardReadSource Read(Backend &backend, u8 *state)
    {
        memset(state, 0, 256);
        if (!backend.IsForeground())
            return Discard(state);
        bool attemptedAcquire = false;
        if (backend.HasDirectInput())
        {
            bool canRead = true;
            if (acquirePending)
            {
                attemptedAcquire = true;
                canRead = backend.Acquire();
            }
            if (canRead && backend.ReadDirectInput(state))
            {
                acquirePending = false;
                return Finish(backend, state, MULTI_KEYBOARD_DIRECTINPUT);
            }
            // Recover an unexpected device loss, then re-read this same frame.
            if (!attemptedAcquire && backend.Acquire() && backend.ReadDirectInput(state))
            {
                acquirePending = false;
                return Finish(backend, state, MULTI_KEYBOARD_DIRECTINPUT);
            }
            acquirePending = true;
        }
        // Failed DirectInput reads may have partially overwritten the buffer.
        memset(state, 0, 256);
        if (!backend.IsForeground())
            return Discard(state);
        backend.ReadFallback(state);
        return Finish(backend, state, MULTI_KEYBOARD_FALLBACK);
    }

  private:
    bool acquirePending;

    MultiKeyboardReadSource Discard(u8 *state)
    {
        acquirePending = true;
        memset(state, 0, 256);
        return MULTI_KEYBOARD_BACKGROUND;
    }

    template <class Backend>
    MultiKeyboardReadSource Finish(Backend &backend, u8 *state, MultiKeyboardReadSource source)
    {
        return backend.IsForeground() ? source : Discard(state);
    }
};

u16 ReadMultiKeyboardButtons();
void ResetMultiKeyboardInput();
void NotifyMultiKeyboardFocusChange();

} // namespace th08
