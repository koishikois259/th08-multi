#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum LauncherControlId
{
    IDC_MODE_HOST = 1001,
    IDC_MODE_GUEST,
    IDC_HOST_ADDRESS,
    IDC_HOST_PORT,
    IDC_LOCAL_PORT,
    IDC_INPUT_DELAY,
    IDC_SAVE,
    IDC_LAUNCH,
    IDC_STATUS,
};

static char g_launcherDirectory[MAX_PATH];
static char g_iniPath[MAX_PATH];
static char g_gamePath[MAX_PATH];
static HFONT g_uiFont;

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

static void LaunchGame(HWND window)
{
    STARTUPINFOA startupInfo;
    PROCESS_INFORMATION processInfo;
    char errorText[192];
    DWORD error;

    if (!SaveSettings(window))
        return;
    if (GetFileAttributesA(g_gamePath) == INVALID_FILE_ATTRIBUTES)
    {
        MessageBoxA(window,
                    "th08-multi.exe was not found next to this launcher.",
                    "Game executable missing", MB_OK | MB_ICONERROR);
        return;
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
        return;
    }

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    SetDlgItemTextA(window, IDC_STATUS, "Game started. Check its title bar for connection status.");
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
            "Internet play: forward the Host UDP port to the Host PC and allow it in the firewall.",
            0, 20, 226, 485, 20, 0);

        CreateLauncherControl(window, "BUTTON", "Save only", BS_PUSHBUTTON | WS_TABSTOP,
                              250, 258, 110, 30, IDC_SAVE);
        CreateLauncherControl(window, "BUTTON", "Save and launch", BS_DEFPUSHBUTTON | WS_TABSTOP,
                              370, 258, 135, 30, IDC_LAUNCH);
        CreateLauncherControl(window, "STATIC", "", SS_LEFT,
                              20, 264, 220, 40, IDC_STATUS);
        LoadSettings(window);
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
            return 0;
        case IDC_SAVE:
            SaveSettings(window);
            return 0;
        case IDC_LAUNCH:
            LaunchGame(window);
            return 0;
        default:
            break;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
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

    window = CreateWindowExA(
        0, windowClass.lpszClassName, "th08-multi v0.1 Network Launcher",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 540, 350,
        NULL, NULL, instance, NULL);
    if (window == NULL)
        return 1;

    ShowWindow(window, showCommand);
    UpdateWindow(window);
    while (GetMessageA(&message, NULL, 0, 0) > 0)
    {
        if (!IsDialogMessageA(window, &message))
        {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
    }
    return static_cast<int>(message.wParam);
}
