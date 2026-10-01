#include "MultiKeyboardInput.hpp"
#include <stdio.h>

using namespace th08;

namespace
{

int failures = 0;
void Expect(bool condition, const char *message)
{
    if (!condition)
    {
        ++failures;
        printf("FAIL: %s\n", message);
    }
}

struct FakeKeyboardBackend
{
    bool foreground;
    bool hasDevice;
    bool acquireSucceeds;
    bool readsSucceed;
    bool loseFocusOnRead;
    bool loseFocusOnFallback;
    int failedReads;
    int acquires;
    int reads;
    int fallbackReads;
    u8 directState[256];
    u8 fallbackState[256];

    FakeKeyboardBackend()
        : foreground(true), hasDevice(true), acquireSucceeds(true), readsSucceed(true),
          loseFocusOnRead(false), loseFocusOnFallback(false), failedReads(0),
          acquires(0), reads(0), fallbackReads(0)
    {
        memset(directState, 0, sizeof(directState));
        memset(fallbackState, 0, sizeof(fallbackState));
    }
    bool IsForeground() { return foreground; }
    bool HasDirectInput() { return hasDevice; }
    bool Acquire() { ++acquires; return acquireSucceeds; }
    bool ReadDirectInput(u8 *state)
    {
        ++reads;
        if (loseFocusOnRead)
            foreground = false;
        if (!readsSucceed || failedReads > 0)
        {
            if (failedReads > 0)
                --failedReads;
            state[DIK_X] = 0x80; // Simulate a partial buffer write on failure.
            return false;
        }
        memcpy(state, directState, sizeof(directState));
        return true;
    }
    void ReadFallback(u8 *state)
    {
        ++fallbackReads;
        // Deliberately do not clear the buffer: the recovery policy must do so.
        for (int index = 0; index < 256; ++index)
            state[index] |= fallbackState[index];
        if (loseFocusOnFallback)
            foreground = false;
    }
};

void TestImmediateRecovery()
{
    MultiKeyboardRecovery recovery;
    FakeKeyboardBackend backend;
    u8 state[256];
    backend.directState[DIK_Z] = backend.directState[DIK_UP] = 0x80;
    Expect(recovery.Read(backend, state) == MULTI_KEYBOARD_DIRECTINPUT &&
               backend.acquires == 1 && backend.reads == 1 &&
               MultiKeyboardStateToButtons(state) == (TH_BUTTON_SHOOT | TH_BUTTON_UP),
           "First foreground sample acquires and immediately reads held keys");

    backend.failedReads = 1;
    Expect(recovery.Read(backend, state) == MULTI_KEYBOARD_DIRECTINPUT &&
               backend.acquires == 2 && backend.reads == 3 && backend.fallbackReads == 0 &&
               MultiKeyboardStateToButtons(state) == (TH_BUTTON_SHOOT | TH_BUTTON_UP),
           "Unexpected input loss reacquires and re-reads during the same sample");

    recovery.OnFocusChanged();
    Expect(recovery.Read(backend, state) == MULTI_KEYBOARD_DIRECTINPUT &&
               backend.acquires == 3 && backend.reads == 4,
           "Activation event triggers acquisition even when no background frame was polled");

    memset(backend.directState, 0, sizeof(backend.directState));
    recovery.Read(backend, state);
    Expect(MultiKeyboardStateToButtons(state) == 0, "Released keys never retain stale input");
}

void TestFallbackAndBoundedRetries()
{
    MultiKeyboardRecovery recovery;
    FakeKeyboardBackend backend;
    u8 state[256];
    backend.acquireSucceeds = false;
    backend.fallbackState[DIK_Z] = backend.fallbackState[DIK_RSHIFT] = 0x80;
    Expect(recovery.Read(backend, state) == MULTI_KEYBOARD_FALLBACK &&
               backend.acquires == 1 && backend.reads == 0 && backend.fallbackReads == 1 &&
               MultiKeyboardStateToButtons(state) == (TH_BUTTON_SHOOT | TH_BUTTON_FOCUS),
           "Acquisition failure uses foreground fallback without waiting in a loop");

    for (int frame = 0; frame < 600; ++frame)
        recovery.Read(backend, state);
    Expect(backend.acquires == 601 && backend.reads == 0 && backend.fallbackReads == 601,
           "Persistent failure makes at most one acquisition attempt per sample");

    backend.acquireSucceeds = true;
    backend.readsSucceed = false;
    Expect(recovery.Read(backend, state) == MULTI_KEYBOARD_FALLBACK &&
               MultiKeyboardStateToButtons(state) == (TH_BUTTON_SHOOT | TH_BUTTON_FOCUS),
           "Failed DirectInput partial buffer contents cannot leak into fallback");

    backend.readsSucceed = true;
    backend.directState[DIK_LEFT] = 0x80;
    Expect(recovery.Read(backend, state) == MULTI_KEYBOARD_DIRECTINPUT &&
               MultiKeyboardStateToButtons(state) == TH_BUTTON_LEFT,
           "DirectInput is retried and recovered after temporary fallback");

    backend.hasDevice = false;
    int acquisitionsBefore = backend.acquires;
    Expect(recovery.Read(backend, state) == MULTI_KEYBOARD_FALLBACK &&
               backend.acquires == acquisitionsBefore,
           "Unavailable or disabled DirectInput uses fallback without dereferencing a device");
}

void TestForegroundBoundary()
{
    MultiKeyboardRecovery recovery;
    FakeKeyboardBackend backend;
    u8 state[256];
    backend.foreground = false;
    backend.directState[DIK_Z] = backend.fallbackState[DIK_Z] = 0x80;
    memset(state, 0xff, sizeof(state));
    Expect(recovery.Read(backend, state) == MULTI_KEYBOARD_BACKGROUND &&
               backend.acquires == 0 && backend.reads == 0 && backend.fallbackReads == 0 &&
               MultiKeyboardStateToButtons(state) == 0,
           "Background/minimized window reads neither input path and clears stale keys");

    backend.foreground = true;
    Expect(recovery.Read(backend, state) == MULTI_KEYBOARD_DIRECTINPUT && backend.acquires == 1,
           "Foreground return automatically reacquires before reading");

    backend.loseFocusOnRead = true;
    Expect(recovery.Read(backend, state) == MULTI_KEYBOARD_BACKGROUND &&
               MultiKeyboardStateToButtons(state) == 0 && backend.fallbackReads == 0,
           "Focus loss during a successful DirectInput read discards the result");

    backend.foreground = true;
    backend.readsSucceed = false;
    Expect(recovery.Read(backend, state) == MULTI_KEYBOARD_BACKGROUND && backend.fallbackReads == 0,
           "Focus loss during a failed read prevents even querying fallback keys");

    backend.foreground = true;
    backend.loseFocusOnRead = false;
    backend.loseFocusOnFallback = true;
    Expect(recovery.Read(backend, state) == MULTI_KEYBOARD_BACKGROUND &&
               MultiKeyboardStateToButtons(state) == 0,
           "Focus loss during fallback polling also discards every sampled key");
}

void TestBindings()
{
    size_t count;
    const MultiKeyboardBinding *bindings = GetMultiKeyboardBindings(&count);
    u8 state[256];
    for (size_t index = 0; index < count; ++index)
    {
        memset(state, 0, sizeof(state));
        state[bindings[index].scanCode] = 0x80;
        Expect(MultiKeyboardStateToButtons(state) == bindings[index].buttons,
               "Every original keyboard action maps identically on both read paths");
        state[bindings[index].scanCode] = 1;
        Expect(MultiKeyboardStateToButtons(state) == 0,
               "Press-history/toggle bits are not treated as held keys");
    }
    memset(state, 0, sizeof(state));
    state[DIK_A] = 0x80;
    Expect(MultiKeyboardStateToButtons(state) == 0, "Unbound keys do not trigger gameplay actions");
}

} // namespace

int main()
{
    TestImmediateRecovery();
    TestFallbackAndBoundedRetries();
    TestForegroundBoundary();
    TestBindings();
    if (failures != 0)
        return 1;
    printf("multiplayer keyboard recovery tests passed\n");
    return 0;
}
