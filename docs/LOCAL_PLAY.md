# Local play (experimental)

This mode runs two co-op players in one `th08-multi.exe` process and one game
window. It does not create a UDP socket or wait for a peer. Gameplay, shared
resources, stage routes, and team choices use the existing multiplayer rules.

Open `th08-multi-launcher.exe`, choose **Local play**,
then select the **Single device** or **Double device** radio option on that page.
The selected option has a green filled circle.
Older configuration files default to **Double device**.

## Single device: shared keyboard

No physical-device discovery or selection is needed. This mode supports a shared
keyboard, not a shared gamepad. P1 uses arrow keys, Shift for focus, Z for
shoot/confirm, and X for bomb/cancel. P2 defaults to I/K/J/L for
up/down/left/right, D for focus, F for shoot/confirm, and G for bomb/cancel.
Esc, Enter and Ctrl remain P1-owned menu/skip controls. Retail auxiliary and
numpad P1 bindings are not merged into this mode, so D cannot operate P1.

Click **Set keys**, click an action in the **Player 1** or **Player 2** column
and press its new key. Shared menu/system keys and duplicate bindings within
either player or across both players are rejected. Esc cancels
capture; **Cancel** discards edits, **Defaults** restores the proposed default
mapping for both players, and **Save** applies both columns. Settings are stored separately under
`[local_keyboard]` in `th08_multi.ini`. Invalid manually edited mappings fall
back to the complete two-player default set. Old P2-only settings retain their
P2 bindings and load the original P1 defaults. Shift aliases are treated as one
key for conflicts; once remapped, old P1 gameplay keys no longer control P1.
These settings do not change online input,
ordinary single-player bindings, or Double device bindings.

Only the foreground, non-minimized game receives input; both players read the
same keyboard snapshot. A keyboard's hardware rollover limits can prevent some
simultaneous key combinations from registering. This mode uses the system
keyboard without physical-device filtering: Windows merges attached keyboards.

## Double device: separate physical devices

Press **Confirm** once on each physical input device, one device at a time,
to reveal it in the device list. On a keyboard, Confirm is **Z**; on a gamepad,
use its confirm/shot button. The launcher accepts any gamepad button for this
identification step to accommodate custom button mappings. This avoids counting
virtual and auxiliary HID interfaces as extra players; gamepad interfaces that
report the same button press together are also merged. Use **Refresh devices**
after plugging in or removing hardware, then identify each device again.
Supported combinations are two keyboards (including an
internal laptop keyboard plus an external keyboard), keyboard plus gamepad,
and two gamepads. Two players cannot share one device in this mode.

P1 controls the main menu and difficulty selection. At team selection, P1
chooses and confirms first; P2's selected device becomes active on the next
frame. In the shared game window both player sprites and both white focus
markers are visible. The local machine's `score.dat` and initial-lives setting
apply to the run. Existing online Host/Guest mode remains separate.

The launcher passes `--multi-mode=local`, `--multi-mode=host`, or
`--multi-mode=guest` to each game process. Local play saves only the device
choices, local input mode and P1/P2 shared-keyboard bindings; it does not replace
an existing online mode. Legacy `mode=local`
residue is cleared when saving local choices and ignored by a direct game
launch without an explicit mode argument. Always use the launcher to start
local play; an online launch never uses the saved local device bindings.

The launcher saves physical device identifiers to `th08_multi.ini` beside the
executables. Do not publish that file without reviewing the identifiers. If a
selected device is unplugged or its identifier changes, return to the launcher
and refresh the list. Raw Input requires Windows; this mode is not available
in the portable Linux build.

Hardware validation still needed before marking this mode stable: shared-keyboard
rollover/focus, custom P1/P2 keys and sequential team selection, two physical keyboards,
internal-plus-external keyboard, keyboard-plus-gamepad, two gamepads, hot-plug
failure handling, team selection/cancel, stage transition, and returning to
the title after a completed run. The current automated multiplayer state and
network tests do not emulate physical HID devices.
