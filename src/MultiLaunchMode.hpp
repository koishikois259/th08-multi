#pragma once

#include <wchar.h>

namespace th08
{

enum MultiPlayerLaunchMode
{
    MULTI_LAUNCH_DISABLED = 0,
    MULTI_LAUNCH_HOST = 1,
    MULTI_LAUNCH_GUEST = 2,
    MULTI_LAUNCH_LOCAL = 3,
};

enum MultiLaunchArgumentStatus
{
    MULTI_LAUNCH_ARGUMENT_MISSING,
    MULTI_LAUNCH_ARGUMENT_VALID,
    MULTI_LAUNCH_ARGUMENT_INVALID,
};

inline const char *GetMultiLaunchModeName(MultiPlayerLaunchMode mode)
{
    switch (mode)
    {
    case MULTI_LAUNCH_HOST: return "host";
    case MULTI_LAUNCH_GUEST: return "guest";
    case MULTI_LAUNCH_LOCAL: return "local";
    default: return "disabled";
    }
}

inline MultiLaunchArgumentStatus ReadMultiLaunchModeArgument(
    int argumentCount, const wchar_t *const *arguments, MultiPlayerLaunchMode *mode)
{
    const wchar_t *prefix = L"--multi-mode=";
    const size_t prefixLength = wcslen(prefix);
    MultiLaunchArgumentStatus status = MULTI_LAUNCH_ARGUMENT_MISSING;
    *mode = MULTI_LAUNCH_DISABLED;
    // argv[0] is the executable path, not a mode argument.
    for (int index = 1; arguments != NULL && index < argumentCount; ++index)
    {
        const wchar_t *argument = arguments[index];
        if (wcscmp(argument, L"--multi-mode") == 0)
            return MULTI_LAUNCH_ARGUMENT_INVALID;
        if (wcsncmp(argument, prefix, prefixLength) != 0)
            continue;
        if (status != MULTI_LAUNCH_ARGUMENT_MISSING)
            return MULTI_LAUNCH_ARGUMENT_INVALID;
        const wchar_t *value = argument + prefixLength;
        if (wcscmp(value, L"host") == 0)
            *mode = MULTI_LAUNCH_HOST;
        else if (wcscmp(value, L"guest") == 0)
            *mode = MULTI_LAUNCH_GUEST;
        else if (wcscmp(value, L"local") == 0)
            *mode = MULTI_LAUNCH_LOCAL;
        else if (wcscmp(value, L"disabled") == 0)
            *mode = MULTI_LAUNCH_DISABLED;
        else
            return MULTI_LAUNCH_ARGUMENT_INVALID;
        status = MULTI_LAUNCH_ARGUMENT_VALID;
    }
    return status;
}

inline MultiPlayerLaunchMode ResolveMultiLaunchMode(
    MultiPlayerLaunchMode storedMode, MultiPlayerLaunchMode argumentMode,
    MultiLaunchArgumentStatus argumentStatus)
{
    if (argumentStatus == MULTI_LAUNCH_ARGUMENT_VALID)
        return argumentMode;
    if (argumentStatus == MULTI_LAUNCH_ARGUMENT_INVALID)
        return MULTI_LAUNCH_DISABLED;
    // Local device filtering is session-only and requires an explicit launch.
    // Preserve legacy direct Host/Guest launches, but never reuse stored local.
    return storedMode == MULTI_LAUNCH_LOCAL ? MULTI_LAUNCH_DISABLED : storedMode;
}

} // namespace th08
