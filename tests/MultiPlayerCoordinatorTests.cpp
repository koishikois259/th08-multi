#include <winsock.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "MultiPlayerCoordinator.hpp"
#include "MultiPlayerCoordinatorTestRuntime.hpp"

namespace th08
{
CoordinatorTestManager g_GameManager;
CoordinatorTestSupervisor g_Supervisor;
CoordinatorTestRng g_Rng;
CoordinatorTestLog g_GameErrorContext;
CoordinatorTestLocalInput g_MultiLocalInput;
u32 hashCalls, hashSeed, hashGameplayFrame;
u32 ComputeCurrentMultiPlayerStateHash()
{
    ++hashCalls;
    hashSeed = g_Rng.seed;
    hashGameplayFrame = g_MultiPlayerState.frameNumber;
    return hashSeed;
}

class MultiPlayerCoordinatorTestAccess
{
  public:
    static bool Open(MultiPlayerCoordinator &c, u16 delay, u8 slot)
    {
        MultiNetWelcomePacket::SaveProgress progress[MULTI_NET_SAVE_SHOT_COUNT] = { 0 };
        c.config.mode = MULTI_LAUNCH_HOST;
        c.initialized = true;
        MultiNetSession &s = c.session;
        if (!s.OpenHost(0, "127.0.0.1", 0, delay, 0x40004, 1, 0x1234, 2, 1, progress))
            return false;
        sockaddr_in address;
        int size = sizeof(address);
        if (getsockname(static_cast<SOCKET>(s.socketHandle),
                        reinterpret_cast<sockaddr *>(&address), &size) != 0)
            return false;
        // Real sendto() targets this bound socket; tests deliver remote samples
        // explicitly so packet loss and worker completion have a fixed order.
        s.remoteAddress = address.sin_addr.s_addr;
        s.remotePort = address.sin_port;
        s.state = MULTI_NET_STATE_CONNECTED;
        s.sessionId = 1;
        s.localSlot = slot;
        s.peerSettingsRevision = s.saveRevision;
        memset(&g_GameManager, 0, sizeof(g_GameManager));
        g_GameManager.cfg = &g_Supervisor.cfg;
        hashCalls = hashSeed = hashGameplayFrame = 0;
        c.BeginGameplay(true, 2, 3, 0);
        return true;
    }
    static void Boundary(MultiPlayerCoordinator &c, u32 frame, u32 start)
    {
        c.networkFrame = frame;
        c.gameplayReadyMask = 3;
        c.gameplayStartFrame = start;
        c.NotifyGameplaySetupComplete();
    }
    static void Feed(MultiPlayerCoordinator &c, u32 frame, u16 buttons = 0,
                     u16 control = 0, u32 epoch = 0)
    {
        c.session.localInputs.Insert(frame, buttons, control, epoch);
        c.session.remoteInputs.Insert(frame, buttons, control, epoch);
    }
    static u32 Frame(const MultiPlayerCoordinator &c) { return c.networkFrame; }
    static u32 Epoch(const MultiPlayerCoordinator &c) { return c.gameplayEpoch; }
    static u8 ReadyMask(const MultiPlayerCoordinator &c) { return c.gameplayReadyMask; }
    static u32 Captured(const MultiPlayerCoordinator &c) { return c.capturedSimulationFrame; }
    static void SetFrame(MultiPlayerCoordinator &c, u32 frame) { c.networkFrame = frame; }
    static void ConfirmSettings(MultiPlayerCoordinator &c, bool confirmed)
    { c.session.peerSettingsRevision = confirmed ? c.session.saveRevision : 0; }
    static MultiNetInputSample Sample(const MultiPlayerCoordinator &c, u32 frame)
    {
        MultiNetInputSample sample = { 0 };
        c.session.localInputs.TryGetSample(frame, &sample);
        return sample;
    }
    static void Remote(MultiPlayerCoordinator &c, u32 frame, u16 buttons = 0,
                       u16 control = 0, u32 epoch = 0)
    { c.session.remoteInputs.Insert(frame, buttons, control, epoch); }
    static void Local(MultiPlayerCoordinator &c)
    { c.config.mode = MULTI_LAUNCH_LOCAL; }
    static bool OpenPair(MultiPlayerCoordinator &host, MultiPlayerCoordinator &guest, u16 delay)
    {
        MultiNetWelcomePacket::SaveProgress progress[MULTI_NET_SAVE_SHOT_COUNT] = { 0 };
        host.config.mode = MULTI_LAUNCH_HOST;
        guest.config.mode = MULTI_LAUNCH_GUEST;
        host.initialized = guest.initialized = true;
        if (!host.session.OpenHost(0, "127.0.0.1", 0, delay, 0x40004, 1, 0x1234, 2, 1, progress))
            return false;
        sockaddr_in address;
        int size = sizeof(address);
        if (getsockname(static_cast<SOCKET>(host.session.socketHandle),
                        reinterpret_cast<sockaddr *>(&address), &size) != 0)
            return false;
        if (!guest.session.OpenGuest(0, "127.0.0.1", "127.0.0.1", ntohs(address.sin_port),
                                    1, delay, 0x40004, 2))
            return false;
        for (u32 tick = 1000; tick < 2000; ++tick)
        {
            host.session.Pump(tick);
            guest.session.Pump(tick);
            if (host.IsConnected() && guest.IsConnected())
                return true;
            Sleep(1);
        }
        return false;
    }
    static void PumpSession(MultiPlayerCoordinator &c, u32 tick) { c.session.Pump(tick); }
};
} // namespace th08

using namespace th08;
typedef MultiPlayerCoordinatorTestAccess Access;
namespace
{
int failures = 0;
void Expect(bool value, const char *message)
{
    if (!value) { ++failures; printf("FAIL: %s\n", message); }
}

void TestFirstSimulationFrame(u16 delay, u8 slot)
{
    MultiPlayerCoordinator c;
    if (!Access::Open(c, delay, slot)) { Expect(false, "test socket opens"); return; }
    Access::Boundary(c, 99, 100);
    Access::Feed(c, 99);
    Access::Feed(c, 100);
    Access::Feed(c, 101);
    u16 p1, p2;
    g_Rng.seed = 0xbeef;
    Expect(c.AcquireGameplayInputs(0, &p1, &p2), "last loading frame consumes inputs");
    Expect(!c.IsGameplaySimulationReady() && g_MultiPlayerState.frameNumber == 0 &&
               g_Rng.seed == 0xbeef && hashCalls == 0,
           "advancing to start frame does not simulate, hash or count early");
    Expect(c.AcquireGameplayInputs(0, &p1, &p2), "first simulation frame consumes inputs");
    Expect(c.IsGameplaySimulationReady() && g_Rng.seed == 0x1234 &&
               g_GameManager.stageRngSeed == 0x1234 && g_MultiPlayerState.frameNumber == 1,
           "first simulation starts with agreed seed and counts once");
    Expect(hashCalls == 1 && hashSeed == 0x1234 && hashGameplayFrame == 0,
           "first hash observes reset RNG before first gameplay update");
    g_Rng.seed = 0x4321; // Simulate the first gameplay tick consuming randomness.
    Expect(c.AcquireGameplayInputs(0, &p1, &p2), "second simulation frame consumes inputs");
    Expect(g_Rng.seed == 0x4321 && g_MultiPlayerState.frameNumber == 2 && hashCalls == 1,
           "second frame does not reset the first tick's RNG advancement");
}

void TestReadinessControls(u16 delay)
{
    MultiPlayerCoordinator c;
    if (!Access::Open(c, delay, 0)) { Expect(false, "test socket opens"); return; }
    const u16 reset = 1 << 14;
    c.EndGameplay();
    c.BeginGameplay(false, 2, 3, 0);
    const u32 epoch = Access::Epoch(c);
    const u32 delayFrames = delay;
    u32 frame;
    u16 p1, p2;
    // Both physical R inputs used to impersonate two completed setup threads.
    for (frame = 0; frame < delayFrames + 5; ++frame)
    {
        Access::Remote(c, frame, reset);
        Expect(c.AcquireGameplayInputs(reset, &p1, &p2), "loading frame progresses");
        Expect(Access::Sample(c, frame + delay).control == MULTI_NET_CONTROL_NONE,
               "physical R cannot advertise loading completion");
    }
    Expect(Access::ReadyMask(c) == 0 && !c.IsGameplaySimulationReady() &&
               g_MultiPlayerState.frameNumber == 0 && hashCalls == 0,
           "unfinished setup never enters simulation or hashes mutable state");
    c.NotifyGameplaySetupComplete();
    Access::ConfirmSettings(c, false);
    frame = Access::Frame(c);
    Access::Remote(c, frame);
    Expect(c.AcquireGameplayInputs(reset, &p1, &p2), "settings wait progresses");
    Expect(Access::Sample(c, frame + delay).control == MULTI_NET_CONTROL_NONE,
           "physical R cannot bypass unconfirmed settings");
    Access::ConfirmSettings(c, true);
    frame = Access::Frame(c);
    Access::Remote(c, frame, 0, MULTI_NET_CONTROL_GAMEPLAY_READY, epoch - 1);
    Expect(c.AcquireGameplayInputs(0, &p1, &p2), "stale readiness frame progresses");
    Expect((Access::ReadyMask(c) & 2) == 0, "previous stage readiness is ignored");
    MultiNetInputSample ready = Access::Sample(c, frame + delay);
    Expect(ready.control == MULTI_NET_CONTROL_GAMEPLAY_READY && ready.gameplayEpoch == epoch &&
               ready.buttons == 0, "readiness is independent from physical buttons");
    for (u32 i = 0; i < 3 * delayFrames + 8; ++i)
    {
        frame = Access::Frame(c);
        Access::Remote(c, frame, 0, MULTI_NET_CONTROL_GAMEPLAY_READY, epoch);
        Expect(c.AcquireGameplayInputs(0, &p1, &p2), "ready and in-flight frames drain");
        if (frame >= delayFrames * 2 + 6)
            Expect((p1 & reset) == 0 && (p2 & reset) == 0,
                   "late readiness controls never leak into R input");
    }
    Expect(c.IsGameplaySimulationReady(), "both real ready signals release the barrier");
    // An actual R press still reaches gameplay after its normal input delay.
    for (u32 i = 0; i <= delay; ++i)
    {
        frame = Access::Frame(c);
        Access::Remote(c, frame, reset);
        c.AcquireGameplayInputs(reset, &p1, &p2);
    }
    Expect((p1 & reset) != 0 && (p2 & reset) != 0, "legitimate R input remains usable");
}

DWORD WINAPI PublishCompletion(void *context)
{
    static_cast<MultiPlayerCoordinator *>(context)->NotifyGameplaySetupComplete();
    return 0;
}

void TestPendingSampleAcrossLifecycle(u16 delay)
{
    MultiPlayerCoordinator c;
    if (!Access::Open(c, delay, 0)) { Expect(false, "test socket opens"); return; }
    Access::SetFrame(c, 20);
    // Current local input exists, but remote frame 20 is deliberately missing.
    u16 p1, p2;
    Expect(!c.AcquireGameplayInputs(1, &p1, &p2), "missing remote input stalls the frame");
    MultiNetInputSample original = Access::Sample(c, 20 + delay);
    c.EndGameplay();
    c.PrepareGameplay(2, 3, 0);
    c.BeginGameplay(false, 2, 3, 0);
    HANDLE worker = CreateThread(NULL, 0, PublishCompletion, &c, 0, NULL);
    Expect(worker != NULL, "completion worker starts");
    if (worker != NULL) { WaitForSingleObject(worker, INFINITE); CloseHandle(worker); }
    Expect(Access::Captured(c) == 20, "lifecycle and worker preserve the captured frame");
    // Supply both current inputs without touching the committed future sample.
    Access::Feed(c, 20);
    Expect(c.AcquireGameplayInputs(0, &p1, &p2), "key release after setup cannot wedge input capture");
    MultiNetInputSample after = Access::Sample(c, 20 + delay);
    Expect(after.buttons == original.buttons && after.control == original.control &&
               after.gameplayEpoch == original.gameplayEpoch && Access::Frame(c) == 21,
           "pending sample is immutable and consumed exactly once");
    Expect(!c.IsGameplaySimulationReady(), "old pending input cannot simulate a new stage");
}

void TestLocalCompletionGate()
{
    MultiPlayerCoordinator c;
    if (!Access::Open(c, 1, 0)) { Expect(false, "test socket opens"); return; }
    Access::Local(c);
    u16 p1, p2;
    c.AcquireGameplayInputs(1, &p1, &p2);
    Expect(!c.IsGameplaySimulationReady(), "local play waits for the worker publication");
    c.NotifyGameplaySetupComplete();
    c.AcquireGameplayInputs(1, &p1, &p2);
    Expect(c.IsGameplaySimulationReady() && g_MultiPlayerState.frameNumber == 1,
           "local play starts on its first complete input tick");
}

void TestTitleControls(u16 delay)
{
    MultiPlayerCoordinator c;
    if (!Access::Open(c, delay, 0)) { Expect(false, "test socket opens"); return; }
    c.EndGameplay();
    c.BeginTitleSynchronization();
    u16 p1, p2;
    u32 i;
    const u32 count = 3 * static_cast<u32>(delay) + 8;
    for (i = 0; i < count; ++i)
    {
        Access::Remote(c, Access::Frame(c), 0x8000);
        Expect(c.AcquireGameplayInputs(0x8000, &p1, &p2), "title wait progresses");
    }
    Expect(!c.IsTitleInputReady(), "physical button bits cannot impersonate title readiness");
    Access::ConfirmSettings(c, true); // A new title snapshot needs its own acknowledgement.
    c.MarkLocalTitleReady();
    for (i = 0; i < count; ++i)
    {
        Access::Remote(c, Access::Frame(c), 0, MULTI_NET_CONTROL_TITLE_READY);
        Expect(c.AcquireGameplayInputs(0, &p1, &p2), "title ready exchange progresses");
        Expect(p1 == 0 && p2 == 0, "title controls never become physical input");
    }
    Expect(c.IsTitleInputReady(), "explicit title readiness releases the title barrier");
    c.EndTitleSynchronization();
    c.PrepareGameplay(2, 3, 0);
    c.BeginGameplay(false, 2, 3, 0);
    Access::Remote(c, Access::Frame(c), 0, MULTI_NET_CONTROL_TITLE_READY);
    Expect(c.AcquireGameplayInputs(0, &p1, &p2), "late title-ready sample drains during stage setup");
    Expect(Access::ReadyMask(c) == 0 && !c.IsGameplaySimulationReady() && p1 == 0 && p2 == 0,
           "late title signal cannot satisfy the gameplay barrier or leak input");
}

// Each peer has its own engine state. Only the coordinator/session/protocol
// implementation is shared; switching these adapters models two game processes.
struct PeerWorld
{
    MultiPlayerState state;
    CoordinatorTestManager manager;
    CoordinatorTestRng rng;
    u32 simulatedTicks;
    u32 firstSimulationFrame;
    u32 results[40];
    void Load()
    {
        g_MultiPlayerState = state;
        g_GameManager = manager;
        g_Rng = rng;
    }
    void Save()
    {
        state = g_MultiPlayerState;
        manager = g_GameManager;
        rng = g_Rng;
    }
};

void TestPairedLoading(u16 delay)
{
    MultiPlayerCoordinator peers[2];
    PeerWorld worlds[2];
    if (!Access::OpenPair(peers[0], peers[1], delay))
    { Expect(false, "paired UDP coordinators connect"); return; }
    u32 tick = 2000;
    for (u32 stage = 0; stage < 2; ++stage)
    {
        for (u32 slot = 0; slot < 2; ++slot)
        {
            if (stage == 0)
            {
                memset(&g_GameManager, 0, sizeof(g_GameManager));
                g_GameManager.cfg = &g_Supervisor.cfg;
                g_MultiPlayerState.Reset(false, 0, 1, 2, 3, 0);
            }
            else
                worlds[slot].Load();
            peers[slot].EndGameplay();
            peers[slot].BeginGameplay(stage == 0, 2, 3, 0);
            // Deliberately distinct loading RNG, just as different loading
            // schedules can leave the visual/shared RNG at different values.
            g_Rng.seed = static_cast<u16>(0xbeef + slot);
            worlds[slot].simulatedTicks = 0;
            worlds[slot].firstSimulationFrame = MULTI_NET_INVALID_FRAME;
            worlds[slot].Save();
        }
        for (u32 attempt = 0; attempt < 2000; ++attempt)
        {
            ++tick;
            for (u32 slot = 0; slot < 2; ++slot)
            {
                Access::PumpSession(peers[slot], tick);
                worlds[slot].Load();
                if (attempt == (slot == stage ? 5u : 65u))
                    peers[slot].NotifyGameplaySetupComplete();
                u16 p1, p2;
                if (worlds[slot].simulatedTicks < 40 &&
                    peers[slot].AcquireGameplayInputs(slot == 0 ? 1 << 14 : 0, &p1, &p2) &&
                    peers[slot].IsGameplaySimulationReady())
                {
                    if (worlds[slot].simulatedTicks == 0)
                        worlds[slot].firstSimulationFrame = Access::Frame(peers[slot]) - 1;
                    g_Rng.seed = static_cast<u16>(g_Rng.seed * 109u + p1 + p2 + 1);
                    worlds[slot].results[worlds[slot].simulatedTicks++] =
                        (static_cast<u32>(g_Rng.seed) << 16) ^ g_MultiPlayerState.frameNumber;
                }
                worlds[slot].Save();
            }
            if (worlds[0].simulatedTicks == 40 && worlds[1].simulatedTicks == 40)
                break;
            if (peers[0].IsSessionFailed() || peers[1].IsSessionFailed())
                break;
            Sleep(1);
        }
        Expect(!peers[0].IsSessionFailed() && !peers[1].IsSessionFailed(),
               "asymmetric loading preserves the UDP session and desync checks");
        Expect(worlds[0].simulatedTicks == 40 && worlds[1].simulatedTicks == 40,
               "both UDP peers finish the paired stage simulation");
        Expect(worlds[0].firstSimulationFrame == worlds[1].firstSimulationFrame &&
                   worlds[0].firstSimulationFrame != MULTI_NET_INVALID_FRAME,
               "fast and slow loaders start on the same consumed network frame");
        Expect(memcmp(worlds[0].results, worlds[1].results, sizeof(worlds[0].results)) == 0 &&
                   worlds[0].state.frameNumber == worlds[1].state.frameNumber,
               "paired RNG and gameplay counters agree through both stage transitions");
    }
}
} // namespace

int main()
{
    const u16 delays[] = { 1, 3, 12 };
    for (u32 i = 0; i < sizeof(delays) / sizeof(delays[0]); ++i)
    {
        TestFirstSimulationFrame(delays[i], 0);
        TestFirstSimulationFrame(delays[i], 1);
        TestReadinessControls(delays[i]);
        TestPendingSampleAcrossLifecycle(delays[i]);
        TestTitleControls(delays[i]);
        TestPairedLoading(delays[i]);
    }
    TestLocalCompletionGate();
    if (failures) { printf("%d coordinator test(s) failed\n", failures); return 1; }
    printf("Multiplayer coordinator tests passed\n");
    return 0;
}
