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
    unsigned char packet[LAUNCHER_PACKET_SIZE];
    unsigned long packetType = 0, packetToken = 0;
    ExpectLauncher(LAUNCHER_PROTOCOL_VERSION == 0x00040003,
                   "launcher version identifies the host-gameplay-settings build");
    WriteLauncherPacket(packet, LAUNCHER_PACKET_HELLO, 123);
    ExpectLauncher(ReadLauncherPacket(packet, sizeof(packet), &packetType, &packetToken) &&
                   packetType == LAUNCHER_PACKET_HELLO && packetToken == 123,
                   "same-version launcher packet accepted");
    unsigned long oldVersion = htonl(0x00040001);
    CopyMemory(packet + 4, &oldVersion, sizeof(oldVersion));
    ExpectLauncher(!ReadLauncherPacket(packet, sizeof(packet), &packetType, &packetToken),
                   "v0.41 launcher packet rejected by v0.42");
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
    ExpectLauncher(IsDlgButtonChecked(g_localWindow, IDC_LOCAL_MODE_SINGLE) == BST_CHECKED &&
                   IsDlgButtonChecked(g_localWindow, IDC_LOCAL_MODE_DOUBLE) == BST_UNCHECKED,
                   "single radio selected exclusively");
    g_singleKeyboardLocal = false;
    UpdateLocalModeOptions(g_localWindow);
    ExpectLauncher(IsDlgButtonChecked(g_localWindow, IDC_LOCAL_MODE_SINGLE) == BST_UNCHECKED &&
                   IsDlgButtonChecked(g_localWindow, IDC_LOCAL_MODE_DOUBLE) == BST_CHECKED,
                   "double radio selected exclusively without device discovery");
    g_singleKeyboardLocal = true;
    UpdateLocalModeOptions(g_localWindow);
    SendMessageA(g_localWindow, WM_COMMAND, IDC_LOCAL_MODE_SINGLE, 0);
    ExpectLauncher(g_singleKeyboardLocal, "clicking selected radio does not toggle mode");
    ExpectLauncher(g_localDeviceCount == 0 && g_gamepadCandidateCount == 0 &&
                   g_keyboardCandidateCount == 0, "single page does not discover devices");
    ExpectLauncher(IsWindowEnabled(GetDlgItem(g_localWindow, IDC_LOCAL_START)) != FALSE,
                   "single mode starts without two devices");
    ExpectLauncher((GetWindowLongA(GetDlgItem(g_localWindow, IDC_LOCAL_P1), GWL_STYLE) & WS_VISIBLE) == 0,
                   "device selectors hidden");
    ExpectLauncher((GetWindowLongA(GetDlgItem(g_localWindow, IDC_LOCAL_KEYS), GWL_STYLE) & WS_VISIBLE) != 0,
                   "both-player settings available");
    // Create without ShowWindow: these tests never display a window or launch a game.
    g_keyWindow = CreateWindowExA(0, "Th08MultiLocalKeyWindow", "Hidden P2 key test",
                                WS_OVERLAPPED, 0, 0, 610, 460,
                                g_localWindow, NULL, type.hInstance, NULL);
    ExpectLauncher(g_keyWindow != NULL, "key page created");
    if (g_keyWindow != NULL)
    {
        ExpectLauncher(GetDlgItem(g_keyWindow, IDC_KEY_FIRST + 6) != NULL &&
                       GetDlgItem(g_keyWindow, IDC_KEY_P2_FIRST + 6) != NULL,
                       "both players have seven editable actions");
        SendMessageA(g_keyWindow, WM_COMMAND, MAKEWPARAM(IDC_KEY_P2_FIRST, BN_SETFOCUS), 0);
        ExpectLauncher(g_keyCaptureAction == -1, "focus notifications do not start key capture");
        SendMessageA(g_keyWindow, WM_COMMAND, IDC_KEY_P2_FIRST, 0);
        SendMessageA(g_keyWindow, WM_COMMAND, MAKEWPARAM(IDC_KEY_P2_FIRST + 1, BN_KILLFOCUS), 0);
        ExpectLauncher(g_keyCaptureAction == 0, "focus notifications do not replace capture action");
        PressCapturedKey('Z', false);
        ExpectLauncher(g_keyCaptureAction == 0 && g_keyDraft.p2Keys[0] == 'I', "P1 conflict stays in capture");
        PressCapturedKey('K', false);
        ExpectLauncher(g_keyCaptureAction == 0 && g_keyDraft.p2Keys[0] == 'I', "P2 duplicate rejected");
        PressCapturedKey('W', true);
        ExpectLauncher(g_keyCaptureAction == 0 && g_keyDraft.p2Keys[0] == 'I', "held-key repeat ignored");
        PressCapturedKey('W', false);
        ExpectLauncher(g_keyCaptureAction == -1 && g_keyDraft.p2Keys[0] == 'W', "valid capture accepted");
        SendMessageA(g_keyWindow, WM_COMMAND, IDC_KEY_P2_FIRST + 1, 0);
        PressCapturedKey(VK_ESCAPE, false);
        ExpectLauncher(g_keyCaptureAction == -1 && g_keyDraft.p2Keys[1] == 'K', "Esc cancels without remapping");
        // Deliver through the real child WndProc, rather than only calling
        // the top-level message filter. No desktop input injection is used.
        SendMessageA(g_keyWindow, WM_COMMAND, IDC_KEY_P2_FIRST, 0);
        HWND upButton = GetDlgItem(g_keyWindow, IDC_KEY_P2_FIRST);
        ExpectLauncher((SendMessageA(upButton, WM_GETDLGCODE, 0, 0) & DLGC_WANTALLKEYS) != 0,
                       "focused capture button bypasses dialog key navigation");
        PostMessageA(upButton, WM_KEYDOWN, VK_PROCESSKEY,
                     static_cast<LPARAM>(MapVirtualKeyA('H', 0)) << 16);
        MSG posted;
        if (PeekMessageA(&posted, upButton, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE))
            DispatchMessageA(&posted);
        else
            ExpectLauncher(false, "posted key reaches child message queue");
        ExpectLauncher(g_keyDraft.p2Keys[0] == 'H' && g_keyCaptureAction == -1,
                       "IME process key maps to physical H through child WndProc");
        RAWKEYBOARD raw;
        ZeroMemory(&raw, sizeof(raw));
        raw.VKey = 'W';
        raw.MakeCode = static_cast<USHORT>(MapVirtualKeyA('W', 0));
        raw.Flags = RI_KEY_BREAK;
        CaptureLocalRawKey(raw);
        SendMessageA(g_keyWindow, WM_COMMAND, IDC_KEY_P2_FIRST, 0);
        CaptureLocalRawKey(raw);
        ExpectLauncher(g_keyCaptureAction == 0, "raw key release never binds a key");
        raw.Flags = 0;
        CaptureLocalRawKey(raw);
        ExpectLauncher(g_keyDraft.p2Keys[0] == 'W' && g_keyCaptureAction == -1,
                       "physical raw key accepts valid W independent of IME");
        SendMessageA(g_keyWindow, WM_COMMAND, IDC_KEY_P2_FIRST + 1, 0);
        CaptureLocalRawKey(raw);
        ExpectLauncher(g_keyCaptureAction == 1 && g_keyDraft.p2Keys[1] == 'K',
                       "held raw key cannot bind next action");
        PressCapturedKey(VK_ESCAPE, false);
        SendMessageA(g_keyWindow, WM_COMMAND, IDC_KEY_FIRST, 0);
        PressCapturedKey('F', false);
        ExpectLauncher(g_keyCaptureSlot == 0 && g_keyCaptureAction == 0 &&
                       g_keyDraft.p1Keys[0] == VK_UP, "P1 cannot reuse P2 key");
        PressCapturedKey('Q', false);
        ExpectLauncher(g_keyDraft.p1Keys[0] == 'Q' && g_keyCaptureAction == -1,
                       "P1 valid key accepted independently");
        SendMessageA(g_keyWindow, WM_COMMAND, IDC_KEY_DEFAULT, 0);
        ExpectLauncher(g_keyDraft.p1Keys[0] == VK_UP && g_keyDraft.p2Keys[0] == 'I',
                       "Defaults restores both player drafts");
        SendMessageA(g_keyWindow, WM_COMMAND, IDC_KEY_FIRST, 0);
        PressCapturedKey('Q', false);
        SendMessageA(g_keyWindow, WM_COMMAND, IDC_KEY_CANCEL, 0);
        th08::MultiLocalKeyboardConfig loaded;
        th08::LoadMultiLocalKeyboardConfig(g_iniPath, &loaded);
        ExpectLauncher(loaded.p2Keys[0] == 'I', "Cancel discards draft");
        ExpectLauncher(loaded.p1Keys[0] == VK_UP, "Cancel discards P1 draft too");
        g_keyWindow = CreateWindowExA(0, "Th08MultiLocalKeyWindow", "Hidden P2 save test",
                                    WS_OVERLAPPED, 0, 0, 610, 460,
                                    g_localWindow, NULL, type.hInstance, NULL);
        if (g_keyWindow != NULL)
        {
            SendMessageA(g_keyWindow, WM_COMMAND, IDC_KEY_P2_FIRST, 0);
            PressCapturedKey('W', false);
            SendMessageA(g_keyWindow, WM_COMMAND, IDC_KEY_FIRST, 0);
            PressCapturedKey('Q', false);
            SendMessageA(g_keyWindow, WM_COMMAND, IDC_KEY_SAVE, 0);
            th08::LoadMultiLocalKeyboardConfig(g_iniPath, &loaded);
            ExpectLauncher(loaded.p2Keys[0] == 'W' && g_keyWindow == NULL, "Save writes draft and closes page");
            ExpectLauncher(loaded.p1Keys[0] == 'Q', "Save writes both player columns");
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
