// Exercise our own hidden launcher windows, not the user's desktop or games.
#include "../src/Th08MultiLauncher.cpp"

static int testFailures = 0;
static void ExpectLauncher(bool condition, const char *name)
{
    if (!condition) { ++testFailures; printf("FAIL: %s\n", name); }
}

static void PressCapturedKey(unsigned int key, bool repeat)
{
    MSG message;
    ZeroMemory(&message, sizeof(message));
    message.hwnd = g_keyWindow;
    message.message = WM_KEYDOWN;
    message.wParam = key;
    message.lParam = repeat ? (1L << 30) : 0;
    ExpectLauncher(CaptureLocalKeyMessage(message), "capture message handled before dialog navigation");
}

int main()
{
    char tempDirectory[MAX_PATH];
    GetTempPathA(sizeof(tempDirectory), tempDirectory);
    if (!GetTempFileNameA(tempDirectory, "t8u", 0, g_iniPath))
        return 1;
    WritePrivateProfileStringA("local", "input_mode", "single", g_iniPath);
    WritePrivateProfileStringA("network", "mode", "guest", g_iniPath);
    WritePrivateProfileStringA("local", "p1_id", "old-double-device", g_iniPath);
    g_uiFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    WNDCLASSEXA type;
    ZeroMemory(&type, sizeof(type));
    type.cbSize = sizeof(type);
    type.hInstance = GetModuleHandleA(NULL);
    type.lpfnWndProc = LocalWindowProc;
    type.lpszClassName = "Th08MultiLocalWindow";
    if (!RegisterClassExA(&type)) return 1;
    type.lpfnWndProc = LocalKeyWindowProc;
    type.lpszClassName = "Th08MultiLocalKeyWindow";
    if (!RegisterClassExA(&type)) return 1;
    g_localWindow = CreateWindowExA(0, "Th08MultiLocalWindow", "Hidden local UI test",
                                  WS_OVERLAPPED, 0, 0, 540, 365,
                                  NULL, NULL, type.hInstance, NULL);
    ExpectLauncher(g_localWindow != NULL, "local page created");
    if (g_localWindow == NULL) { DeleteFileA(g_iniPath); return 1; }
    ExpectLauncher(g_singleKeyboardLocal, "single mode selected from isolated local setting");
    ExpectLauncher(g_localDeviceCount == 0 && g_gamepadCandidateCount == 0 &&
                   g_keyboardCandidateCount == 0, "single page does not discover devices");
    ExpectLauncher(IsWindowEnabled(GetDlgItem(g_localWindow, IDC_LOCAL_START)) != FALSE,
                   "single mode starts without two devices");
    ExpectLauncher((GetWindowLongA(GetDlgItem(g_localWindow, IDC_LOCAL_P1), GWL_STYLE) & WS_VISIBLE) == 0,
                   "device selectors hidden");
    ExpectLauncher((GetWindowLongA(GetDlgItem(g_localWindow, IDC_LOCAL_KEYS), GWL_STYLE) & WS_VISIBLE) != 0,
                   "P2 settings available");
    // Create without ShowWindow: these tests never display a window or launch a game.
    g_keyWindow = CreateWindowExA(0, "Th08MultiLocalKeyWindow", "Hidden P2 key test",
                                WS_OVERLAPPED, 0, 0, 415, 420,
                                g_localWindow, NULL, type.hInstance, NULL);
    ExpectLauncher(g_keyWindow != NULL, "key page created");
    if (g_keyWindow != NULL)
    {
        SendMessageA(g_keyWindow, WM_COMMAND, IDC_KEY_FIRST, 0);
        PressCapturedKey('Z', false);
        ExpectLauncher(g_keyCaptureAction == 0 && g_keyDraft.p2Keys[0] == 'I', "P1 conflict stays in capture");
        PressCapturedKey('K', false);
        ExpectLauncher(g_keyCaptureAction == 0 && g_keyDraft.p2Keys[0] == 'I', "P2 duplicate rejected");
        PressCapturedKey('W', true);
        ExpectLauncher(g_keyCaptureAction == 0 && g_keyDraft.p2Keys[0] == 'I', "held-key repeat ignored");
        PressCapturedKey('W', false);
        ExpectLauncher(g_keyCaptureAction == -1 && g_keyDraft.p2Keys[0] == 'W', "valid capture accepted");
        SendMessageA(g_keyWindow, WM_COMMAND, IDC_KEY_FIRST + 1, 0);
        PressCapturedKey(VK_ESCAPE, false);
        ExpectLauncher(g_keyCaptureAction == -1 && g_keyDraft.p2Keys[1] == 'K', "Esc cancels without remapping");
        SendMessageA(g_keyWindow, WM_COMMAND, IDC_KEY_CANCEL, 0);
        th08::MultiLocalKeyboardConfig loaded;
        th08::LoadMultiLocalKeyboardConfig(g_iniPath, &loaded);
        ExpectLauncher(loaded.p2Keys[0] == 'I', "Cancel discards draft");
        g_keyWindow = CreateWindowExA(0, "Th08MultiLocalKeyWindow", "Hidden P2 save test",
                                    WS_OVERLAPPED, 0, 0, 415, 420,
                                    g_localWindow, NULL, type.hInstance, NULL);
        if (g_keyWindow != NULL)
        {
            SendMessageA(g_keyWindow, WM_COMMAND, IDC_KEY_FIRST, 0);
            PressCapturedKey('W', false);
            SendMessageA(g_keyWindow, WM_COMMAND, IDC_KEY_SAVE, 0);
            th08::LoadMultiLocalKeyboardConfig(g_iniPath, &loaded);
            ExpectLauncher(loaded.p2Keys[0] == 'W' && g_keyWindow == NULL, "Save writes draft and closes page");
        }
    }
    ExpectLauncher(SaveLocalSelection(g_localWindow), "single launch settings bypass device selection");
    char text[64];
    GetPrivateProfileStringA("network", "mode", "", text, sizeof(text), g_iniPath);
    ExpectLauncher(strcmp(text, "guest") == 0, "local launch save preserves remote mode");
    GetPrivateProfileStringA("local", "p1_id", "", text, sizeof(text), g_iniPath);
    ExpectLauncher(strcmp(text, "old-double-device") == 0, "single launch save preserves double devices");
    DestroyWindow(g_localWindow);
    DeleteFileA(g_iniPath);
    printf("Local launcher tests: %s\n", testFailures == 0 ? "PASS" : "FAIL");
    return testFailures != 0;
}
