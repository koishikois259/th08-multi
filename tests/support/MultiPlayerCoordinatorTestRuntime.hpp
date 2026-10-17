#pragma once

// Engine/device adapters for compiling the production coordinator without D3D.
// Input history, wire encoding, session and coordinator logic remain real.
#include <windows.h>
#include "inttypes.hpp"
#include "MultiPlayerState.hpp"
#include "SecureRandom.hpp"

#ifndef C_ASSERT
#define C_ASSERT(e) typedef char test_compile_assert[(e) ? 1 : -1]
#endif

namespace th08
{
enum { SHOT_REIMU_YUKARI = 0, SHOT_ALL = 12, MAX_DIFFICULTIES = 5, NORMAL = 1, EXTRA = 4 };
struct CoordinatorTestConfiguration { u8 lifeCount; u8 slowMode; };
struct CoordinatorTestManager
{
    struct Progress
    {
        u16 difficultiesClearedWithoutRetries[MAX_DIFFICULTIES];
        u16 difficultiesClearedWithRetries[MAX_DIFFICULTIES];
        bool pendingEndingSkip;
    } clrdData[SHOT_ALL + 1];
    struct Flags
    {
        bool isExtraUnlocked, isSpellPracticeUnlocked, isExtraUnlockedWithAllTeams;
        bool isPracticeMode, isSpellPractice;
    } flags;
    CoordinatorTestConfiguration *cfg;
    int difficulty;
    u16 stageRngSeed;
    bool IsExtraUnlocked() const { return false; }
    bool IsSpellPracticeUnlocked() const { return false; }
    bool IsExtraUnlockedForCharacter(int) const { return false; }
    bool IsExtraUnlockedWithAllTeams() const { return false; }
};
struct CoordinatorTestSupervisor
{
    HWND hwndGameWindow;
    CoordinatorTestConfiguration cfg;
};
struct CoordinatorTestRng { u16 seed; };
struct CoordinatorTestLog { void Log(const char *, ...) {} };
struct CoordinatorTestLocalInput
{
    u16 buttons[2];
    bool Initialize(HWND) { return true; }
    bool RebindWindow(HWND) { return true; }
    void Shutdown() {}
    u16 GetInput(u8 slot) { return buttons[slot]; }
};
struct Controller { static u16 GetInput() { return 0; } };
extern CoordinatorTestManager g_GameManager;
extern CoordinatorTestSupervisor g_Supervisor;
extern CoordinatorTestRng g_Rng;
extern CoordinatorTestLog g_GameErrorContext;
extern CoordinatorTestLocalInput g_MultiLocalInput;
u32 ComputeCurrentMultiPlayerStateHash();
} // namespace th08
