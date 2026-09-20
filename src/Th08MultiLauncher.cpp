#include <windows.h>
#include <winsock.h>
#include <dinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SecureRandom.hpp"

enum LauncherControlId
{
    IDC_MODE_HOST = 1001,
    IDC_MODE_GUEST,
    IDC_HOST_ADDRESS,
    IDC_HOST_PORT,
    IDC_LOCAL_PORT,
    IDC_INPUT_DELAY,
    IDC_CONNECT,
    IDC_LAUNCH,
    IDC_STATUS,
    IDC_LOCAL_PAGE,
    IDC_LOCAL_P1,
    IDC_LOCAL_P2,
    IDC_LOCAL_REFRESH,
    IDC_LOCAL_BACK,
    IDC_LOCAL_START,
    IDC_LOCAL_STATUS,
};

enum LocalDeviceType
{
    LOCAL_DEVICE_KEYBOARD,
    LOCAL_DEVICE_GAMEPAD,
};

struct LocalDevice
{
    LocalDeviceType type;
    char identifier[512];
    char label[160];
    HANDLE rawHandle;
};

enum LauncherConnectionState
{
    LAUNCHER_CONNECTION_IDLE,
    LAUNCHER_CONNECTION_HOST_WAITING,
    LAUNCHER_CONNECTION_GUEST_CONNECTING,
    LAUNCHER_CONNECTION_CONNECTED,
    LAUNCHER_CONNECTION_STARTING,
};

enum LauncherPacketType
{
    LAUNCHER_PACKET_HELLO = 1,
    LAUNCHER_PACKET_ACK,
    LAUNCHER_PACKET_KEEPALIVE,
    LAUNCHER_PACKET_START,
    LAUNCHER_PACKET_START_ACK,
};

static const unsigned long LAUNCHER_PACKET_MAGIC = 0x54384D4C;
static const unsigned long LAUNCHER_PROTOCOL_VERSION = 0x00030004;
static const int LAUNCHER_PACKET_SIZE = 16;
static const UINT_PTR LAUNCHER_TIMER_ID = 1;
static const UINT LAUNCHER_TIMER_INTERVAL_MS = 100;
static const DWORD LAUNCHER_SEND_INTERVAL_MS = 500;
static const DWORD LAUNCHER_START_SEND_INTERVAL_MS = 100;
static const DWORD LAUNCHER_CONNECTION_TIMEOUT_MS = 5000;
static const DWORD LAUNCHER_GUEST_LAUNCH_DELAY_MS = 750;
static const DWORD LAUNCHER_HOST_LAUNCH_DELAY_MS = 250;
static const unsigned int LAUNCHER_MAX_PACKETS_PER_TICK = 32;

static char g_launcherDirectory[MAX_PATH];
static char g_iniPath[MAX_PATH];
static char g_gamePath[MAX_PATH];
static HFONT g_uiFont;
static SOCKET g_launcherSocket = INVALID_SOCKET;
static LauncherConnectionState g_connectionState = LAUNCHER_CONNECTION_IDLE;
static sockaddr_in g_peerAddress;
static bool g_hasPeerAddress = false;
static bool g_isHost = false;
static bool g_winsockStarted = false;
static unsigned long g_sessionToken = 0;
static DWORD g_lastSendTime = 0;
static DWORD g_lastReceiveTime = 0;
static DWORD g_startRequestTime = 0;
static DWORD g_launchAt = 0;
static HWND g_mainWindow = NULL;
static HWND g_localWindow = NULL;
static LocalDevice g_localDevices[64];
static int g_localDeviceCount = 0;
static LocalDevice g_keyboardCandidates[64];
static int g_keyboardCandidateCount = 0;

static void FormatGamepadGuid(const GUID &guid, char *text)
{
    wsprintfA(text,
              "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
              guid.Data1, guid.Data2, guid.Data3,
              guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
              guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

static BOOL CALLBACK EnumerateLocalGamepad(const DIDEVICEINSTANCEA *device, VOID *)
{
    LocalDevice *entry;
    int prefixLength;
    if (g_localDeviceCount >= static_cast<int>(sizeof(g_localDevices) / sizeof(g_localDevices[0])))
        return DIENUM_STOP;
    entry = &g_localDevices[g_localDeviceCount++];
    entry->type = LOCAL_DEVICE_GAMEPAD;
    entry->rawHandle = NULL;
    FormatGamepadGuid(device->guidInstance, entry->identifier);
    wsprintfA(entry->label, "Gamepad %d: ", g_localDeviceCount);
    prefixLength = lstrlenA(entry->label);
    lstrcpynA(entry->label + prefixLength, device->tszInstanceName,
              sizeof(entry->label) - prefixLength);
    return DIENUM_CONTINUE;
}

static void EnumerateLocalDevices()
{
    UINT count = 0;
    RAWINPUTDEVICELIST *devices;
    UINT index;
    LPDIRECTINPUT8A directInput = NULL;
    g_localDeviceCount = 0;
    g_keyboardCandidateCount = 0;
    if (GetRawInputDeviceList(NULL, &count, sizeof(RAWINPUTDEVICELIST)) != (UINT)-1 && count != 0)
    {
        devices = static_cast<RAWINPUTDEVICELIST *>(malloc(sizeof(RAWINPUTDEVICELIST) * count));
        if (devices != NULL)
        {
            if (GetRawInputDeviceList(devices, &count, sizeof(RAWINPUTDEVICELIST)) != (UINT)-1)
            {
                for (index = 0; index < count &&
                     g_keyboardCandidateCount < static_cast<int>(sizeof(g_keyboardCandidates) / sizeof(g_keyboardCandidates[0]));
                     ++index)
                {
                    LocalDevice *entry;
                    UINT nameLength;
                    if (devices[index].dwType != RIM_TYPEKEYBOARD)
                        continue;
                    // Raw Input reports auxiliary HID keyboard collections,
                    // mouse hotkey interfaces and virtual keyboards too. Keep
                    // them as candidates, but show a keyboard only after a
                    // real gameplay key comes from that exact device handle.
                    entry = &g_keyboardCandidates[g_keyboardCandidateCount];
                    nameLength = sizeof(entry->identifier);
                    if (GetRawInputDeviceInfoA(devices[index].hDevice, RIDI_DEVICENAME,
                                               entry->identifier, &nameLength) == (UINT)-1)
                        continue;
                    entry->type = LOCAL_DEVICE_KEYBOARD;
                    entry->rawHandle = devices[index].hDevice;
                    ++g_keyboardCandidateCount;
                }
            }
            free(devices);
        }
    }
    if (SUCCEEDED(DirectInput8Create(GetModuleHandleA(NULL), DIRECTINPUT_VERSION,
                                     IID_IDirectInput8A,
                                     reinterpret_cast<void **>(&directInput), NULL)))
    {
        directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumerateLocalGamepad,
                                 NULL, DIEDFL_ATTACHEDONLY);
        directInput->Release();
    }
}

static void RefreshLocalDeviceControls(HWND window)
{
    HWND p1 = GetDlgItem(window, IDC_LOCAL_P1);
    HWND p2 = GetDlgItem(window, IDC_LOCAL_P2);
    char savedType[16];
    char savedId[512];
    char key[16];
    int slot;
    int index;
    int savedIndex[2] = {-1, -1};
    RAWINPUTDEVICE keyboard;
    EnumerateLocalDevices();
    SendMessageA(p1, CB_RESETCONTENT, 0, 0);
    SendMessageA(p2, CB_RESETCONTENT, 0, 0);
    for (index = 0; index < g_localDeviceCount; ++index)
    {
        SendMessageA(p1, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(g_localDevices[index].label));
        SendMessageA(p2, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(g_localDevices[index].label));
    }
    for (slot = 0; slot < 2; ++slot)
    {
        wsprintfA(key, "p%d_type", slot + 1);
        GetPrivateProfileStringA("local", key, "", savedType, sizeof(savedType), g_iniPath);
        wsprintfA(key, "p%d_id", slot + 1);
        GetPrivateProfileStringA("local", key, "", savedId, sizeof(savedId), g_iniPath);
        for (index = 0; index < g_localDeviceCount; ++index)
        {
            const char *type = g_localDevices[index].type == LOCAL_DEVICE_KEYBOARD
                                   ? "keyboard" : "gamepad";
            if (lstrcmpiA(savedType, type) == 0 &&
                lstrcmpiA(savedId, g_localDevices[index].identifier) == 0)
            {
                savedIndex[slot] = index;
                break;
            }
        }
    }
    if (savedIndex[0] < 0 && g_localDeviceCount > 0)
        savedIndex[0] = 0;
    if (savedIndex[1] < 0 && g_localDeviceCount > 1)
        savedIndex[1] = savedIndex[0] == 0 ? 1 : 0;
    SendMessageA(p1, CB_SETCURSEL, savedIndex[0], 0);
    SendMessageA(p2, CB_SETCURSEL, savedIndex[1], 0);
    SetDlgItemTextA(window, IDC_LOCAL_STATUS,
                    "Press Z on each keyboard to identify it. Gamepads appear automatically.");
    EnableWindow(GetDlgItem(window, IDC_LOCAL_START), g_localDeviceCount >= 2);
    keyboard.usUsagePage = 1;
    keyboard.usUsage = 6;
    keyboard.dwFlags = 0;
    keyboard.hwndTarget = window;
    RegisterRawInputDevices(&keyboard, 1, sizeof(keyboard));
}

static void IdentifyLocalKeyboard(HWND window, HRAWINPUT input)
{
    RAWINPUT raw;
    UINT size = sizeof(raw);
    char status[160];
    int index;
    int keyboardNumber;
    LocalDevice *entry;
    if (GetRawInputData(input, RID_INPUT, &raw, &size,
                        sizeof(RAWINPUTHEADER)) == (UINT)-1 ||
        raw.header.dwType != RIM_TYPEKEYBOARD ||
        (raw.data.keyboard.Flags & RI_KEY_BREAK) != 0 ||
        (raw.data.keyboard.MakeCode & 0x7f) != DIK_Z ||
        (raw.data.keyboard.Flags & RI_KEY_E0) != 0)
        return;
    for (index = 0; index < g_localDeviceCount; ++index)
    {
        if (g_localDevices[index].type == LOCAL_DEVICE_KEYBOARD &&
            g_localDevices[index].rawHandle == raw.header.hDevice)
        {
            wsprintfA(status, "Keyboard %d is already detected. Press Z on the other keyboard.",
                      index + 1);
            SetDlgItemTextA(window, IDC_LOCAL_STATUS, status);
            return;
        }
    }
    for (index = 0; index < g_keyboardCandidateCount; ++index)
    {
        if (g_keyboardCandidates[index].rawHandle != raw.header.hDevice)
            continue;
        if (g_localDeviceCount >= static_cast<int>(sizeof(g_localDevices) / sizeof(g_localDevices[0])))
            return;
        keyboardNumber = 1;
        for (int existing = 0; existing < g_localDeviceCount; ++existing)
            if (g_localDevices[existing].type == LOCAL_DEVICE_KEYBOARD)
                ++keyboardNumber;
        entry = &g_localDevices[g_localDeviceCount];
        *entry = g_keyboardCandidates[index];
        wsprintfA(entry->label, "Keyboard %d (Z key detected)", keyboardNumber);
        SendMessageA(GetDlgItem(window, IDC_LOCAL_P1), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(entry->label));
        SendMessageA(GetDlgItem(window, IDC_LOCAL_P2), CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(entry->label));
        if (SendMessageA(GetDlgItem(window, IDC_LOCAL_P1), CB_GETCURSEL, 0, 0) == CB_ERR)
            SendMessageA(GetDlgItem(window, IDC_LOCAL_P1), CB_SETCURSEL, g_localDeviceCount, 0);
        else if (SendMessageA(GetDlgItem(window, IDC_LOCAL_P2), CB_GETCURSEL, 0, 0) == CB_ERR)
            SendMessageA(GetDlgItem(window, IDC_LOCAL_P2), CB_SETCURSEL, g_localDeviceCount, 0);
        ++g_localDeviceCount;
        EnableWindow(GetDlgItem(window, IDC_LOCAL_START), g_localDeviceCount >= 2);
        wsprintfA(status, "Keyboard %d detected. Choose P1/P2 devices, or press Z on another keyboard.",
                  keyboardNumber);
        SetDlgItemTextA(window, IDC_LOCAL_STATUS, status);
        return;
    }
    SetDlgItemTextA(window, IDC_LOCAL_STATUS,
                    "Keyboard changed. Click Refresh devices, then press Z again.");
}

static HWND CreateLauncherControl(
    HWND parent, const char *className, const char *text, DWORD style,
    int x, int y, int width, int height, int id)
{
    HWND control = CreateWindowExA(
        0, className, text, WS_CHILD | WS_VISIBLE | style,
        x, y, width, height, parent,
        reinterpret_cast<HMENU>(id), GetModuleHandleA(NULL), NULL);
    if (control != NULL)
        SendMessageA(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
    return control;
}

static void InitializePaths()
{
    char *slash;
    const char *iniName = "\\th08_multi.ini";
    const char *gameName = "\\th08-multi.exe";
    DWORD pathLength;

    pathLength = GetModuleFileNameA(
        NULL, g_launcherDirectory, sizeof(g_launcherDirectory));
    if (pathLength == 0 || pathLength >= sizeof(g_launcherDirectory))
    {
        lstrcpyA(g_launcherDirectory, ".");
    }
    else
    {
        slash = strrchr(g_launcherDirectory, '\\');
        if (slash != NULL)
            *slash = '\0';
        else
            lstrcpyA(g_launcherDirectory, ".");
    }

    lstrcpynA(g_iniPath, g_launcherDirectory,
              sizeof(g_iniPath) - lstrlenA(iniName));
    lstrcatA(g_iniPath, iniName);
    lstrcpynA(g_gamePath, g_launcherDirectory,
              sizeof(g_gamePath) - lstrlenA(gameName));
    lstrcatA(g_gamePath, gameName);
}

static void SetIntegerControl(HWND window, int id, unsigned int value)
{
    char text[32];
    wsprintfA(text, "%u", value);
    SetDlgItemTextA(window, id, text);
}

static bool ReadIntegerControl(
    HWND window, int id, unsigned int minimum, unsigned int maximum,
    const char *fieldName, unsigned int *value)
{
    char text[32];
    char message[160];
    char *end;
    unsigned long parsed;

    GetDlgItemTextA(window, id, text, sizeof(text));
    end = NULL;
    parsed = strtoul(text, &end, 10);
    if (text[0] == '\0' || end == NULL || *end != '\0' ||
        parsed < minimum || parsed > maximum)
    {
        wsprintfA(message, "%s must be between %u and %u.",
                  fieldName, minimum, maximum);
        MessageBoxA(window, message, "Invalid network setting", MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(window, id));
        return false;
    }

    *value = static_cast<unsigned int>(parsed);
    return true;
}

static void LoadSettings(HWND window)
{
    char mode[16];
    char host[64];
    unsigned int hostPort;
    unsigned int localPort;
    unsigned int inputDelay;

    GetPrivateProfileStringA("network", "mode", "host", mode, sizeof(mode), g_iniPath);
    GetPrivateProfileStringA("network", "host", "127.0.0.1", host, sizeof(host), g_iniPath);
    hostPort = GetPrivateProfileIntA("network", "host_port", 17708, g_iniPath);
    localPort = GetPrivateProfileIntA(
        "network", "local_port", lstrcmpiA(mode, "guest") == 0 ? 0 : 17708, g_iniPath);
    inputDelay = GetPrivateProfileIntA("network", "input_delay", 3, g_iniPath);

    CheckRadioButton(
        window, IDC_MODE_HOST, IDC_MODE_GUEST,
        lstrcmpiA(mode, "guest") == 0 ? IDC_MODE_GUEST : IDC_MODE_HOST);
    SetDlgItemTextA(window, IDC_HOST_ADDRESS, host);
    SetIntegerControl(window, IDC_HOST_PORT, hostPort);
    SetIntegerControl(window, IDC_LOCAL_PORT, localPort);
    SetIntegerControl(window, IDC_INPUT_DELAY, inputDelay);
}

static bool SaveSettings(HWND window)
{
    char host[64];
    char number[32];
    const char *mode;
    unsigned int hostPort;
    unsigned int localPort;
    unsigned int inputDelay;
    bool isHost;

    isHost = IsDlgButtonChecked(window, IDC_MODE_HOST) == BST_CHECKED;
    mode = isHost ? "host" : "guest";
    GetDlgItemTextA(window, IDC_HOST_ADDRESS, host, sizeof(host));
    if (!isHost && (host[0] == '\0' || inet_addr(host) == INADDR_NONE))
    {
        MessageBoxA(window, "Guest mode requires a valid Host IPv4 address.",
                    "Invalid network setting", MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(window, IDC_HOST_ADDRESS));
        return false;
    }
    if (!ReadIntegerControl(window, IDC_HOST_PORT, 1, 65535, "Host port", &hostPort))
        return false;
    if (!ReadIntegerControl(window, IDC_LOCAL_PORT, isHost ? 1 : 0, 65535,
                            "Local port", &localPort))
        return false;
    if (!ReadIntegerControl(window, IDC_INPUT_DELAY, 1, 12, "Input delay", &inputDelay))
        return false;

    wsprintfA(number, "%u", hostPort);
    if (!WritePrivateProfileStringA("network", "mode", mode, g_iniPath) ||
        !WritePrivateProfileStringA("network", "host", host, g_iniPath) ||
        !WritePrivateProfileStringA("network", "bind_address", "0.0.0.0", g_iniPath) ||
        !WritePrivateProfileStringA("network", "host_port", number, g_iniPath))
    {
        MessageBoxA(window, "Could not write th08_multi.ini next to the launcher.",
                    "Save failed", MB_OK | MB_ICONERROR);
        return false;
    }

    wsprintfA(number, "%u", localPort);
    if (!WritePrivateProfileStringA("network", "local_port", number, g_iniPath))
    {
        MessageBoxA(window, "Could not write th08_multi.ini next to the launcher.",
                    "Save failed", MB_OK | MB_ICONERROR);
        return false;
    }
    wsprintfA(number, "%u", inputDelay);
    if (!WritePrivateProfileStringA("network", "input_delay", number, g_iniPath))
    {
        MessageBoxA(window, "Could not write th08_multi.ini next to the launcher.",
                    "Save failed", MB_OK | MB_ICONERROR);
        return false;
    }
    WritePrivateProfileStringA(NULL, NULL, NULL, g_iniPath);
    SetDlgItemTextA(window, IDC_STATUS, "Configuration saved.");
    return true;
}

static bool LaunchGame(HWND window)
{
    STARTUPINFOA startupInfo;
    PROCESS_INFORMATION processInfo;
    char errorText[192];
    DWORD error;

    if (GetFileAttributesA(g_gamePath) == INVALID_FILE_ATTRIBUTES)
    {
        MessageBoxA(window,
                    "th08-multi.exe was not found next to this launcher.",
                    "Game executable missing", MB_OK | MB_ICONERROR);
        return false;
    }

    ZeroMemory(&startupInfo, sizeof(startupInfo));
    ZeroMemory(&processInfo, sizeof(processInfo));
    startupInfo.cb = sizeof(startupInfo);
    if (!CreateProcessA(g_gamePath, NULL, NULL, NULL, FALSE, 0, NULL,
                        g_launcherDirectory, &startupInfo, &processInfo))
    {
        error = GetLastError();
        wsprintfA(errorText, "Could not start th08-multi.exe (Windows error %lu).", error);
        MessageBoxA(window, errorText, "Launch failed", MB_OK | MB_ICONERROR);
        return false;
    }

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return true;
}

static bool SaveLocalSelection(HWND window)
{
    int p1 = static_cast<int>(SendMessageA(GetDlgItem(window, IDC_LOCAL_P1),
                                          CB_GETCURSEL, 0, 0));
    int p2 = static_cast<int>(SendMessageA(GetDlgItem(window, IDC_LOCAL_P2),
                                          CB_GETCURSEL, 0, 0));
    const LocalDevice *first;
    const LocalDevice *second;
    if (p1 < 0 || p2 < 0 || p1 >= g_localDeviceCount || p2 >= g_localDeviceCount)
    {
        MessageBoxA(window, "Select two connected input devices.",
                    "Local play input", MB_OK | MB_ICONWARNING);
        return false;
    }
    first = &g_localDevices[p1];
    second = &g_localDevices[p2];
    if (first->type == second->type &&
        lstrcmpiA(first->identifier, second->identifier) == 0)
    {
        MessageBoxA(window, "P1 and P2 must use different physical devices.",
                    "Local play input", MB_OK | MB_ICONWARNING);
        return false;
    }
    if (!WritePrivateProfileStringA("local", "p1_type",
                                    first->type == LOCAL_DEVICE_KEYBOARD ? "keyboard" : "gamepad",
                                    g_iniPath) ||
        !WritePrivateProfileStringA("local", "p1_id", first->identifier, g_iniPath) ||
        !WritePrivateProfileStringA("local", "p2_type",
                                    second->type == LOCAL_DEVICE_KEYBOARD ? "keyboard" : "gamepad",
                                    g_iniPath) ||
        !WritePrivateProfileStringA("local", "p2_id", second->identifier, g_iniPath) ||
        !WritePrivateProfileStringA("network", "mode", "local", g_iniPath))
    {
        MessageBoxA(window, "Could not save th08_multi.ini beside the launcher.",
                    "Local play setup failed", MB_OK | MB_ICONERROR);
        return false;
    }
    WritePrivateProfileStringA(NULL, NULL, NULL, g_iniPath);
    return true;
}

static LRESULT CALLBACK LocalWindowProc(HWND window, UINT message,
                                        WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INPUT:
        IdentifyLocalKeyboard(window, reinterpret_cast<HRAWINPUT>(lParam));
        break;
    case WM_CREATE:
        CreateLauncherControl(window, "STATIC", "Local play - two input devices, one game window", 0,
                              20, 17, 490, 22, 0);
        CreateLauncherControl(window, "STATIC", "Player 1:", 0,
                              20, 61, 105, 20, 0);
        CreateLauncherControl(window, "COMBOBOX", "",
                              CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                              130, 55, 375, 250, IDC_LOCAL_P1);
        CreateLauncherControl(window, "STATIC", "Player 2:", 0,
                              20, 105, 105, 20, 0);
        CreateLauncherControl(window, "COMBOBOX", "",
                              CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                              130, 99, 375, 250, IDC_LOCAL_P2);
        CreateLauncherControl(window, "STATIC",
                              "Press Z on each keyboard to detect it. Gamepads appear automatically.",
                              0, 20, 147, 490, 20, 0);
        CreateLauncherControl(window, "STATIC", "", SS_LEFT,
                              20, 176, 490, 38, IDC_LOCAL_STATUS);
        CreateLauncherControl(window, "BUTTON", "Refresh devices", WS_TABSTOP,
                              20, 235, 135, 30, IDC_LOCAL_REFRESH);
        CreateLauncherControl(window, "BUTTON", "Back", WS_TABSTOP,
                              260, 235, 90, 30, IDC_LOCAL_BACK);
        CreateLauncherControl(window, "BUTTON", "Start local game", BS_DEFPUSHBUTTON | WS_TABSTOP,
                              350, 235, 155, 30, IDC_LOCAL_START);
        RefreshLocalDeviceControls(window);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_LOCAL_REFRESH:
            RefreshLocalDeviceControls(window);
            return 0;
        case IDC_LOCAL_BACK:
            DestroyWindow(window);
            ShowWindow(g_mainWindow, SW_SHOW);
            return 0;
        case IDC_LOCAL_START:
            if (SaveLocalSelection(window) && LaunchGame(window))
            {
                DestroyWindow(window);
                DestroyWindow(g_mainWindow);
            }
            return 0;
        default:
            break;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(window);
        ShowWindow(g_mainWindow, SW_SHOW);
        return 0;
    case WM_DESTROY:
        {
            RAWINPUTDEVICE keyboard;
            keyboard.usUsagePage = 1;
            keyboard.usUsage = 6;
            keyboard.dwFlags = RIDEV_REMOVE;
            keyboard.hwndTarget = NULL;
            RegisterRawInputDevices(&keyboard, 1, sizeof(keyboard));
        }
        g_localWindow = NULL;
        return 0;
    default:
        break;
    }
    return DefWindowProcA(window, message, wParam, lParam);
}

static void OpenLocalPage(HWND mainWindow)
{
    if (g_localWindow != NULL)
        return;
    g_localWindow = CreateWindowExA(
        0, "Th08MultiLocalWindow", "th08-multi Local play",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 540, 315,
        NULL, NULL, GetModuleHandleA(NULL), NULL);
    if (g_localWindow != NULL)
    {
        ShowWindow(mainWindow, SW_HIDE);
        ShowWindow(g_localWindow, SW_SHOW);
        UpdateWindow(g_localWindow);
    }
}

static void SetSocketErrorStatus(HWND window, const char *operation)
{
    char text[192];
    wsprintfA(text, "%s failed (Windows socket error %d).", operation, WSAGetLastError());
    SetDlgItemTextA(window, IDC_STATUS, text);
}

static void SetConnectionControls(HWND window)
{
    bool active = g_connectionState != LAUNCHER_CONNECTION_IDLE;
    bool hostCanLaunch = g_isHost &&
                         g_connectionState == LAUNCHER_CONNECTION_CONNECTED;
    SetDlgItemTextA(window, IDC_CONNECT, active ? "Disconnect" : "Connect");
    EnableWindow(GetDlgItem(window, IDC_MODE_HOST), !active);
    EnableWindow(GetDlgItem(window, IDC_MODE_GUEST), !active);
    EnableWindow(GetDlgItem(window, IDC_HOST_ADDRESS), !active);
    EnableWindow(GetDlgItem(window, IDC_HOST_PORT), !active);
    EnableWindow(GetDlgItem(window, IDC_LOCAL_PORT), !active);
    EnableWindow(GetDlgItem(window, IDC_INPUT_DELAY), !active);
    EnableWindow(GetDlgItem(window, IDC_LAUNCH), hostCanLaunch);
    EnableWindow(GetDlgItem(window, IDC_LOCAL_PAGE), !active);
}

static void CloseLauncherConnection(HWND window)
{
    if (g_launcherSocket != INVALID_SOCKET)
    {
        closesocket(g_launcherSocket);
        g_launcherSocket = INVALID_SOCKET;
    }
    if (g_winsockStarted)
    {
        WSACleanup();
        g_winsockStarted = false;
    }
    g_connectionState = LAUNCHER_CONNECTION_IDLE;
    g_hasPeerAddress = false;
    g_sessionToken = 0;
    g_lastSendTime = 0;
    g_lastReceiveTime = 0;
    g_startRequestTime = 0;
    g_launchAt = 0;
    if (window != NULL)
        SetConnectionControls(window);
}

static void WriteLauncherPacket(unsigned char *data, unsigned long type,
                                unsigned long token)
{
    unsigned long value;
    value = htonl(LAUNCHER_PACKET_MAGIC);
    CopyMemory(data, &value, sizeof(value));
    value = htonl(LAUNCHER_PROTOCOL_VERSION);
    CopyMemory(data + 4, &value, sizeof(value));
    value = htonl(type);
    CopyMemory(data + 8, &value, sizeof(value));
    value = htonl(token);
    CopyMemory(data + 12, &value, sizeof(value));
}

static bool ReadLauncherPacket(const unsigned char *data, int size,
                               unsigned long *type, unsigned long *token)
{
    unsigned long value;
    if (data == NULL || type == NULL || token == NULL || size != LAUNCHER_PACKET_SIZE)
        return false;
    CopyMemory(&value, data, sizeof(value));
    if (ntohl(value) != LAUNCHER_PACKET_MAGIC)
        return false;
    CopyMemory(&value, data + 4, sizeof(value));
    if (ntohl(value) != LAUNCHER_PROTOCOL_VERSION)
        return false;
    CopyMemory(&value, data + 8, sizeof(value));
    *type = ntohl(value);
    if (*type < LAUNCHER_PACKET_HELLO || *type > LAUNCHER_PACKET_START_ACK)
        return false;
    CopyMemory(&value, data + 12, sizeof(value));
    *token = ntohl(value);
    return true;
}

static bool IsSameEndpoint(const sockaddr_in &left, const sockaddr_in &right)
{
    return left.sin_addr.s_addr == right.sin_addr.s_addr &&
           left.sin_port == right.sin_port;
}

static void SendLauncherPacket(unsigned long type)
{
    unsigned char data[LAUNCHER_PACKET_SIZE];
    if (g_launcherSocket == INVALID_SOCKET || !g_hasPeerAddress)
        return;
    WriteLauncherPacket(data, type, g_sessionToken);
    sendto(g_launcherSocket, reinterpret_cast<const char *>(data), sizeof(data), 0,
           reinterpret_cast<const sockaddr *>(&g_peerAddress), sizeof(g_peerAddress));
    g_lastSendTime = GetTickCount();
}

static bool StartLauncherConnection(HWND window)
{
    WSADATA data;
    sockaddr_in localAddress;
    unsigned long nonBlocking;
    unsigned int hostPort;
    unsigned int localPort;
    unsigned int inputDelay;
    char hostText[64];
    char statusText[192];

    if (!SaveSettings(window))
        return false;
    g_isHost = IsDlgButtonChecked(window, IDC_MODE_HOST) == BST_CHECKED;
    GetDlgItemTextA(window, IDC_HOST_ADDRESS, hostText, sizeof(hostText));
    if (!ReadIntegerControl(window, IDC_HOST_PORT, 1, 65535, "Host port", &hostPort) ||
        !ReadIntegerControl(window, IDC_LOCAL_PORT, g_isHost ? 1 : 0, 65535,
                            "Local port", &localPort) ||
        !ReadIntegerControl(window, IDC_INPUT_DELAY, 1, 12, "Input delay", &inputDelay))
        return false;

    if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
    {
        SetSocketErrorStatus(window, "Network initialization");
        return false;
    }
    g_winsockStarted = true;
    g_launcherSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (g_launcherSocket == INVALID_SOCKET)
    {
        SetSocketErrorStatus(window, "Socket creation");
        CloseLauncherConnection(window);
        return false;
    }

    ZeroMemory(&localAddress, sizeof(localAddress));
    localAddress.sin_family = AF_INET;
    localAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddress.sin_port = htons(static_cast<unsigned short>(localPort));
    if (bind(g_launcherSocket, reinterpret_cast<const sockaddr *>(&localAddress),
             sizeof(localAddress)) == SOCKET_ERROR)
    {
        SetSocketErrorStatus(window, "UDP port binding");
        CloseLauncherConnection(window);
        return false;
    }
    nonBlocking = 1;
    if (ioctlsocket(g_launcherSocket, FIONBIO, &nonBlocking) == SOCKET_ERROR)
    {
        SetSocketErrorStatus(window, "Non-blocking socket setup");
        CloseLauncherConnection(window);
        return false;
    }

    ZeroMemory(&g_peerAddress, sizeof(g_peerAddress));
    g_peerAddress.sin_family = AF_INET;
    g_lastReceiveTime = GetTickCount();
    if (g_isHost)
    {
        if (!th08::GenerateSecureRandomNonZeroU32(&g_sessionToken))
        {
            MessageBoxA(window, "Windows secure random generation failed. Connection was not opened.",
                        "Security initialization failed", MB_OK | MB_ICONERROR);
            CloseLauncherConnection(window);
            return false;
        }
        g_connectionState = LAUNCHER_CONNECTION_HOST_WAITING;
        g_hasPeerAddress = false;
        wsprintfA(statusText, "Waiting for Guest on UDP %u...", localPort);
    }
    else
    {
        g_peerAddress.sin_addr.s_addr = inet_addr(hostText);
        g_peerAddress.sin_port = htons(static_cast<unsigned short>(hostPort));
        g_hasPeerAddress = true;
        g_sessionToken = 0;
        g_connectionState = LAUNCHER_CONNECTION_GUEST_CONNECTING;
        g_lastSendTime = 0;
        wsprintfA(statusText, "Connecting to Host %s:%u...", hostText, hostPort);
    }
    SetDlgItemTextA(window, IDC_STATUS, statusText);
    SetConnectionControls(window);
    return true;
}

static void HandleLauncherPacket(HWND window, const sockaddr_in &sender,
                                 unsigned long type, unsigned long token,
                                 DWORD now)
{
    if (g_isHost)
    {
        if (type == LAUNCHER_PACKET_HELLO && token == 0 &&
            (!g_hasPeerAddress || IsSameEndpoint(sender, g_peerAddress)))
        {
            g_peerAddress = sender;
            g_hasPeerAddress = true;
            g_lastReceiveTime = now;
            SendLauncherPacket(LAUNCHER_PACKET_ACK);
            if (g_connectionState != LAUNCHER_CONNECTION_STARTING)
            {
                g_connectionState = LAUNCHER_CONNECTION_CONNECTED;
                SetDlgItemTextA(window, IDC_STATUS,
                                "Connected. Host may now start both games.");
                SetConnectionControls(window);
            }
        }
        else if (g_hasPeerAddress && IsSameEndpoint(sender, g_peerAddress) &&
                 token == g_sessionToken)
        {
            g_lastReceiveTime = now;
            if (type == LAUNCHER_PACKET_KEEPALIVE)
                SendLauncherPacket(LAUNCHER_PACKET_ACK);
            else if (type == LAUNCHER_PACKET_START_ACK &&
                     g_connectionState == LAUNCHER_CONNECTION_STARTING &&
                     g_launchAt == 0)
            {
                g_launchAt = now + LAUNCHER_HOST_LAUNCH_DELAY_MS;
                SetDlgItemTextA(window, IDC_STATUS,
                                "Guest is ready. Launching both games...");
            }
        }
    }
    else if (g_hasPeerAddress && IsSameEndpoint(sender, g_peerAddress))
    {
        if (type == LAUNCHER_PACKET_ACK && token != 0)
        {
            g_sessionToken = token;
            g_lastReceiveTime = now;
            if (g_connectionState != LAUNCHER_CONNECTION_STARTING)
            {
                g_connectionState = LAUNCHER_CONNECTION_CONNECTED;
                SetDlgItemTextA(window, IDC_STATUS,
                                "Connected. Waiting for Host to start the game.");
                SetConnectionControls(window);
            }
        }
        else if (type == LAUNCHER_PACKET_START && token == g_sessionToken &&
                 g_sessionToken != 0)
        {
            g_lastReceiveTime = now;
            SendLauncherPacket(LAUNCHER_PACKET_START_ACK);
            if (g_connectionState != LAUNCHER_CONNECTION_STARTING)
            {
                g_connectionState = LAUNCHER_CONNECTION_STARTING;
                g_launchAt = now + LAUNCHER_GUEST_LAUNCH_DELAY_MS;
                SetDlgItemTextA(window, IDC_STATUS,
                                "Host started the session. Launching game...");
                SetConnectionControls(window);
            }
        }
    }
}

static void PumpLauncherConnection(HWND window)
{
    unsigned char data[LAUNCHER_PACKET_SIZE];
    sockaddr_in sender;
    int senderSize;
    int received;
    int error;
    unsigned long type;
    unsigned long token;
    unsigned int packetCount;
    DWORD now = GetTickCount();

    if (g_launcherSocket == INVALID_SOCKET)
        return;
    for (packetCount = 0; packetCount < LAUNCHER_MAX_PACKETS_PER_TICK; ++packetCount)
    {
        senderSize = sizeof(sender);
        received = recvfrom(g_launcherSocket, reinterpret_cast<char *>(data),
                            sizeof(data), 0, reinterpret_cast<sockaddr *>(&sender),
                            &senderSize);
        if (received == SOCKET_ERROR)
        {
            error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK)
                SetSocketErrorStatus(window, "Receiving launcher packet");
            break;
        }
        if (ReadLauncherPacket(data, received, &type, &token))
            HandleLauncherPacket(window, sender, type, token, now);
    }

    if (g_connectionState == LAUNCHER_CONNECTION_GUEST_CONNECTING &&
        now - g_lastSendTime >= LAUNCHER_SEND_INTERVAL_MS)
        SendLauncherPacket(LAUNCHER_PACKET_HELLO);
    else if (g_connectionState == LAUNCHER_CONNECTION_CONNECTED && !g_isHost &&
             now - g_lastSendTime >= LAUNCHER_SEND_INTERVAL_MS)
        SendLauncherPacket(LAUNCHER_PACKET_KEEPALIVE);
    else if (g_connectionState == LAUNCHER_CONNECTION_STARTING && g_isHost &&
             g_launchAt == 0 &&
             now - g_lastSendTime >= LAUNCHER_START_SEND_INTERVAL_MS)
        SendLauncherPacket(LAUNCHER_PACKET_START);
    else if (g_connectionState == LAUNCHER_CONNECTION_STARTING && !g_isHost &&
             now - g_lastSendTime >= LAUNCHER_START_SEND_INTERVAL_MS)
        SendLauncherPacket(LAUNCHER_PACKET_START_ACK);

    if (g_connectionState == LAUNCHER_CONNECTION_CONNECTED &&
        now - g_lastReceiveTime > LAUNCHER_CONNECTION_TIMEOUT_MS)
    {
        if (g_isHost)
        {
            g_connectionState = LAUNCHER_CONNECTION_HOST_WAITING;
            g_hasPeerAddress = false;
            SetDlgItemTextA(window, IDC_STATUS,
                            "Guest disconnected. Waiting for reconnection...");
        }
        else
        {
            g_connectionState = LAUNCHER_CONNECTION_GUEST_CONNECTING;
            g_sessionToken = 0;
            g_lastSendTime = 0;
            SetDlgItemTextA(window, IDC_STATUS,
                            "Host connection lost. Reconnecting...");
        }
        SetConnectionControls(window);
    }

    if (g_connectionState == LAUNCHER_CONNECTION_STARTING && g_isHost &&
        g_launchAt == 0 && now - g_startRequestTime > LAUNCHER_CONNECTION_TIMEOUT_MS)
    {
        g_connectionState = LAUNCHER_CONNECTION_CONNECTED;
        SetDlgItemTextA(window, IDC_STATUS,
                        "Guest did not acknowledge start. Try again.");
        SetConnectionControls(window);
    }

    if (g_connectionState == LAUNCHER_CONNECTION_STARTING &&
        g_launchAt != 0 && static_cast<long>(now - g_launchAt) >= 0)
    {
        CloseLauncherConnection(window);
        if (LaunchGame(window))
            DestroyWindow(window);
    }
}

static void GetLocalIpv4Text(char *output, int outputSize)
{
    WSADATA data;
    char hostName[256];
    struct hostent *host;
    int index;
    bool found;

    lstrcpynA(output, "Local IPv4: unavailable", outputSize);
    if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
        return;
    if (gethostname(hostName, sizeof(hostName)) != 0)
    {
        WSACleanup();
        return;
    }
    host = gethostbyname(hostName);
    if (host == NULL)
    {
        WSACleanup();
        return;
    }

    lstrcpynA(output, "Local IPv4: ", outputSize);
    found = false;
    for (index = 0; index < 3 && host->h_addr_list[index] != NULL; ++index)
    {
        struct in_addr address;
        const char *addressText;
        CopyMemory(&address, host->h_addr_list[index], sizeof(address));
        addressText = inet_ntoa(address);
        if (addressText == NULL)
            continue;
        if (found && lstrlenA(output) + 2 < outputSize)
            lstrcatA(output, ", ");
        if (lstrlenA(output) + lstrlenA(addressText) + 1 < outputSize)
        {
            lstrcatA(output, addressText);
            found = true;
        }
    }
    if (!found)
        lstrcpynA(output, "Local IPv4: unavailable", outputSize);
    WSACleanup();
}

static LRESULT CALLBACK LauncherWindowProc(
    HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    char localAddressText[256];

    switch (message)
    {
    case WM_CREATE:
        g_uiFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        CreateLauncherControl(window, "STATIC", "Connection mode:", 0,
                              20, 20, 120, 20, 0);
        CreateLauncherControl(window, "BUTTON", "Host", BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP,
                              150, 17, 80, 24, IDC_MODE_HOST);
        CreateLauncherControl(window, "BUTTON", "Guest", BS_AUTORADIOBUTTON | WS_TABSTOP,
                              240, 17, 80, 24, IDC_MODE_GUEST);

        CreateLauncherControl(window, "STATIC", "Host IPv4 address:", 0,
                               20, 58, 125, 20, 0);
        CreateLauncherControl(window, "EDIT", "", WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                               150, 54, 220, 24, IDC_HOST_ADDRESS);

        CreateLauncherControl(window, "STATIC", "Host UDP port:", 0,
                              20, 94, 125, 20, 0);
        CreateLauncherControl(window, "EDIT", "", WS_BORDER | WS_TABSTOP | ES_NUMBER,
                              150, 90, 90, 24, IDC_HOST_PORT);

        CreateLauncherControl(window, "STATIC", "Local UDP port:", 0,
                              20, 130, 125, 20, 0);
        CreateLauncherControl(window, "EDIT", "", WS_BORDER | WS_TABSTOP | ES_NUMBER,
                              150, 126, 90, 24, IDC_LOCAL_PORT);
        CreateLauncherControl(window, "STATIC", "Host: same as host port. Guest: 0 is automatic.", 0,
                              250, 130, 255, 20, 0);

        CreateLauncherControl(window, "STATIC", "Input delay (1-12):", 0,
                              20, 166, 125, 20, 0);
        CreateLauncherControl(window, "EDIT", "", WS_BORDER | WS_TABSTOP | ES_NUMBER,
                              150, 162, 90, 24, IDC_INPUT_DELAY);
        CreateLauncherControl(window, "STATIC", "Use the same value on both PCs; start with 3.", 0,
                              250, 166, 255, 20, 0);

        GetLocalIpv4Text(localAddressText, sizeof(localAddressText));
        CreateLauncherControl(window, "STATIC", localAddressText, 0,
                              20, 202, 485, 20, 0);
        CreateLauncherControl(
            window, "STATIC",
            "UDP listens on all local IPv4 interfaces; connect only to a trusted peer.",
            0, 20, 226, 485, 20, 0);

        CreateLauncherControl(window, "BUTTON", "Connect", BS_DEFPUSHBUTTON | WS_TABSTOP,
                              250, 258, 110, 30, IDC_CONNECT);
        CreateLauncherControl(window, "BUTTON", "Start both games", BS_PUSHBUTTON | WS_TABSTOP,
                              370, 258, 135, 30, IDC_LAUNCH);
        CreateLauncherControl(window, "STATIC", "", SS_LEFT,
                              20, 264, 220, 40, IDC_STATUS);
        CreateLauncherControl(window, "BUTTON", "Local play (multi input)",
                              BS_PUSHBUTTON | WS_TABSTOP,
                              330, 309, 175, 30, IDC_LOCAL_PAGE);
        LoadSettings(window);
        SetConnectionControls(window);
        SetTimer(window, LAUNCHER_TIMER_ID, LAUNCHER_TIMER_INTERVAL_MS, NULL);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_MODE_HOST:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                char localPort[32];
                GetDlgItemTextA(window, IDC_LOCAL_PORT, localPort, sizeof(localPort));
                if (lstrcmpA(localPort, "0") == 0 || localPort[0] == '\0')
                {
                    char hostPort[32];
                    GetDlgItemTextA(window, IDC_HOST_PORT, hostPort, sizeof(hostPort));
                    SetDlgItemTextA(window, IDC_LOCAL_PORT, hostPort);
                }
            }
            return 0;
        case IDC_MODE_GUEST:
            if (HIWORD(wParam) == BN_CLICKED)
                SetDlgItemTextA(window, IDC_LOCAL_PORT, "0");
            return 0;
        case IDC_CONNECT:
            if (g_connectionState == LAUNCHER_CONNECTION_IDLE)
                StartLauncherConnection(window);
            else
            {
                CloseLauncherConnection(window);
                SetDlgItemTextA(window, IDC_STATUS, "Disconnected.");
            }
            return 0;
        case IDC_LAUNCH:
            if (g_isHost &&
                g_connectionState == LAUNCHER_CONNECTION_CONNECTED)
            {
                g_connectionState = LAUNCHER_CONNECTION_STARTING;
                g_startRequestTime = GetTickCount();
                g_launchAt = 0;
                g_lastSendTime = 0;
                SetDlgItemTextA(window, IDC_STATUS,
                                "Sending start command to Guest...");
                SetConnectionControls(window);
                SendLauncherPacket(LAUNCHER_PACKET_START);
            }
            return 0;
        case IDC_LOCAL_PAGE:
            if (g_connectionState == LAUNCHER_CONNECTION_IDLE)
                OpenLocalPage(window);
            return 0;
        default:
            break;
        }
        break;

    case WM_TIMER:
        if (wParam == LAUNCHER_TIMER_ID)
            PumpLauncherConnection(window);
        return 0;

    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        KillTimer(window, LAUNCHER_TIMER_ID);
        CloseLauncherConnection(NULL);
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProcA(window, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand)
{
    WNDCLASSEXA windowClass;
    WNDCLASSEXA localClass;
    HWND window;
    MSG message;

    InitializePaths();
    ZeroMemory(&windowClass, sizeof(windowClass));
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = LauncherWindowProc;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    windowClass.lpszClassName = "Th08MultiLauncherWindow";
    windowClass.hIconSm = windowClass.hIcon;
    if (!RegisterClassExA(&windowClass))
        return 1;

    localClass = windowClass;
    localClass.lpfnWndProc = LocalWindowProc;
    localClass.lpszClassName = "Th08MultiLocalWindow";
    if (!RegisterClassExA(&localClass))
        return 1;

    window = CreateWindowExA(
        0, windowClass.lpszClassName, "th08-multi v0.34 Network Launcher",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 540, 400,
        NULL, NULL, instance, NULL);
    if (window == NULL)
        return 1;
    g_mainWindow = window;

    ShowWindow(window, showCommand);
    UpdateWindow(window);
    while (GetMessageA(&message, NULL, 0, 0) > 0)
    {
        if (!IsDialogMessageA(g_localWindow != NULL ? g_localWindow : window, &message))
        {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
    }
    return static_cast<int>(message.wParam);
}
