# Local play (development build)

This mode runs two co-op players in one `th08-multi.exe` process and one game
window. It does not create a UDP socket or wait for a peer. Gameplay, shared
resources, stage routes, and team choices use the existing multiplayer rules.

Open `th08-multi-launcher.exe`, choose **Local play (multi input device)**,
then assign two different physical devices. The page lists attached Raw Input
keyboards and DirectInput game controllers. Pressing a key identifies which
listed keyboard generated it. Use **Refresh devices** after plugging in or
removing hardware. Supported combinations are two keyboards (including an
internal laptop keyboard plus an external keyboard), keyboard plus gamepad,
and two gamepads. Two players cannot share one device in this mode.

P1 controls the main menu and difficulty selection. At team selection, P1
chooses and confirms first; P2's selected device becomes active on the next
frame. In the shared game window both player sprites and both white focus
markers are visible. The local machine's `score.dat` and initial-lives setting
apply to the run. Existing online Host/Guest mode remains separate.

The launcher saves physical device identifiers to `th08_multi.ini` beside the
executables. Do not publish that file without reviewing the identifiers. If a
selected device is unplugged or its identifier changes, return to the launcher
and refresh the list. Raw Input requires Windows; this mode is not available
in the portable Linux build.

Hardware validation still needed before release: two physical keyboards,
internal-plus-external keyboard, keyboard-plus-gamepad, two gamepads, hot-plug
failure handling, team selection/cancel, stage transition, and returning to
the title after a completed run. The current automated multiplayer state and
network tests do not emulate physical HID devices.
