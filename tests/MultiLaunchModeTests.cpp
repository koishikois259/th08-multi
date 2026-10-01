#include <windows.h>
#include <shellapi.h>
#include <stdio.h>

#include "MultiLaunchMode.hpp"

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

void ExpectCommandLine(const wchar_t *commandLine, MultiPlayerLaunchMode storedMode,
                       MultiLaunchArgumentStatus expectedStatus,
                       MultiPlayerLaunchMode expectedMode, const char *message)
{
    int argumentCount = 0;
    LPWSTR *arguments = CommandLineToArgvW(commandLine, &argumentCount);
    Expect(arguments != NULL, "Windows command-line parsing succeeds");
    if (arguments == NULL)
        return;
    MultiPlayerLaunchMode argumentMode;
    MultiLaunchArgumentStatus status =
        ReadMultiLaunchModeArgument(argumentCount, arguments, &argumentMode);
    Expect(status == expectedStatus &&
               ResolveMultiLaunchMode(storedMode, argumentMode, status) == expectedMode,
           message);
    LocalFree(arguments);
}

void TestModeIsolation()
{
    const MultiPlayerLaunchMode modes[] = {
        MULTI_LAUNCH_DISABLED, MULTI_LAUNCH_HOST, MULTI_LAUNCH_GUEST, MULTI_LAUNCH_LOCAL,
    };
    for (int index = 0; index < 4; ++index)
    {
        ExpectCommandLine(L"\"C:\\Touhou Game\\th08-multi.exe\" --multi-mode=host", modes[index],
                          MULTI_LAUNCH_ARGUMENT_VALID, MULTI_LAUNCH_HOST,
                          "Host launch ignores any stored mode, including local");
        ExpectCommandLine(L"\"C:\\Touhou Game\\th08-multi.exe\" --multi-mode=guest", modes[index],
                          MULTI_LAUNCH_ARGUMENT_VALID, MULTI_LAUNCH_GUEST,
                          "Guest launch ignores any stored mode, including local");
        ExpectCommandLine(L"\"C:\\Touhou Game\\th08-multi.exe\" --multi-mode=local", modes[index],
                          MULTI_LAUNCH_ARGUMENT_VALID, MULTI_LAUNCH_LOCAL,
                          "Local play requires and honors the explicit local argument");
        ExpectCommandLine(L"game.exe --multi-mode=disabled", modes[index],
                          MULTI_LAUNCH_ARGUMENT_VALID, MULTI_LAUNCH_DISABLED,
                          "Explicit disabled mode overrides stored online/local mode");
    }
    ExpectCommandLine(L"game.exe", MULTI_LAUNCH_LOCAL,
                      MULTI_LAUNCH_ARGUMENT_MISSING, MULTI_LAUNCH_DISABLED,
                      "Direct EXE launch cannot inherit local-device filtering");
    ExpectCommandLine(L"game.exe", MULTI_LAUNCH_HOST,
                      MULTI_LAUNCH_ARGUMENT_MISSING, MULTI_LAUNCH_HOST,
                      "Legacy direct Host launch remains supported");
    ExpectCommandLine(L"game.exe", MULTI_LAUNCH_GUEST,
                      MULTI_LAUNCH_ARGUMENT_MISSING, MULTI_LAUNCH_GUEST,
                      "Legacy direct Guest launch remains supported");
    ExpectCommandLine(L"game.exe", MULTI_LAUNCH_DISABLED,
                      MULTI_LAUNCH_ARGUMENT_MISSING, MULTI_LAUNCH_DISABLED,
                      "No mode argument preserves ordinary single-player mode");
}

void TestArgumentBoundaries()
{
    ExpectCommandLine(L"\"C:\\--multi-mode=local\\game.exe\" --other=local", MULTI_LAUNCH_HOST,
                      MULTI_LAUNCH_ARGUMENT_MISSING, MULTI_LAUNCH_HOST,
                      "Executable paths and unrelated arguments are not mode switches");
    ExpectCommandLine(L"game.exe \"--multi-mode=guest\"", MULTI_LAUNCH_LOCAL,
                      MULTI_LAUNCH_ARGUMENT_VALID, MULTI_LAUNCH_GUEST,
                      "Quoted mode arguments are parsed by Windows");
    ExpectCommandLine(L"game.exe --other=value --multi-mode=host", MULTI_LAUNCH_LOCAL,
                      MULTI_LAUNCH_ARGUMENT_VALID, MULTI_LAUNCH_HOST,
                      "Mode argument need not be the first argument");
    ExpectCommandLine(L"game.exe --multi-mode=host --multi-mode=local", MULTI_LAUNCH_LOCAL,
                      MULTI_LAUNCH_ARGUMENT_INVALID, MULTI_LAUNCH_DISABLED,
                      "Conflicting explicit modes cannot enable local filtering");
    ExpectCommandLine(L"game.exe --multi-mode=host --multi-mode=host", MULTI_LAUNCH_LOCAL,
                      MULTI_LAUNCH_ARGUMENT_INVALID, MULTI_LAUNCH_DISABLED,
                      "Duplicate modes are rejected");
    ExpectCommandLine(L"game.exe --multi-mode=local-extra", MULTI_LAUNCH_LOCAL,
                      MULTI_LAUNCH_ARGUMENT_INVALID, MULTI_LAUNCH_DISABLED,
                      "Mode values require an exact match");
    ExpectCommandLine(L"game.exe --multi-mode=", MULTI_LAUNCH_LOCAL,
                      MULTI_LAUNCH_ARGUMENT_INVALID, MULTI_LAUNCH_DISABLED,
                      "Empty modes do not fall back to stored local");
    ExpectCommandLine(L"game.exe --multi-mode", MULTI_LAUNCH_LOCAL,
                      MULTI_LAUNCH_ARGUMENT_INVALID, MULTI_LAUNCH_DISABLED,
                      "Missing mode values do not fall back to stored local");
}

} // namespace

int main()
{
    TestModeIsolation();
    TestArgumentBoundaries();
    if (failures != 0)
        return 1;
    printf("multiplayer launch-mode tests passed\n");
    return 0;
}
