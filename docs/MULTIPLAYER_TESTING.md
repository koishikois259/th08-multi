# th08-multi v0.4 manual multiplayer testing

`th08-multi-launcher.exe` writes `th08_multi.ini`, establishes the pre-game
connection, and starts `th08-multi.exe` on both peers after the Host issues the
start command. Keep both executables in the same directory as the legally
obtained TH08 1.00d data files.

The distributable `th08-multi-v0.4-patch.zip` contains no original game data.
Extract all of its files directly into a Japanese TH08 1.00d directory, then
start `th08-multi-launcher.exe`. The original `th08.exe` remains untouched.

## Trusted LAN/VPN test

On the Host PC:

1. Start `th08-multi-launcher.exe`.
2. Select **Host**.
3. The **Local IPv4** row is informational; there is no Bind input field.
4. Set Host UDP port and Local UDP port to the same value. The default is
   `17708`.
5. Choose **Connect** and wait for the status to report `Connected`.
6. After the Guest is connected, choose **Start both games**.

On the Guest PC:

1. Start the launcher and select **Guest**.
2. Enter the Host PC's address on the same trusted network or tunnel.
3. Enter the Host's UDP port. Local UDP port may remain `0` for automatic
   selection.
4. Set the same input delay as the Host; start with `3`.
5. Choose **Connect** and wait for the Host. The Guest start button remains
   disabled; the game starts when the Host chooses **Start both games**.

The launcher reports waiting, connecting, connected, and synchronized-start
states. After launch, the game window title reports listening, connecting,
connected, team-select, playing, and network-error states. Both computers must
use the same v0.4 build and the same Japanese TH08 1.00d game data. The title
menu remains input-locked until both game instances have finished loading it.

For an initial-lives sync check, set different initial lives in the two local
game configurations before connecting. Both players should start with the
Host/P1 value; the Guest/P2 setting must not affect the run. On both screens,
check that P1 and P2 LIFE/BOMB marks are visible and that the remote character
does not show a white focus hitbox.

## Local-to-online input isolation check

1. Start local play with two physical devices, then exit the game normally.
2. Reopen the launcher and connect as Host or Guest for a two-PC online run.
3. Check `log.txt` for `multi launch: mode=host source=command-line` or
   `mode=guest source=command-line`. There must be no
   `multi local: two physical input devices ready` line in an online run.
4. Verify team selection and gameplay input on both machines. P1 still
   controls the shared title menu; Guest title-menu navigation is not expected.
5. Repeat with an old `mode=local` configuration left in place before online
   launch. The explicit online mode must win over that residue.
6. Local play must still work when the stored network mode is Host or Guest,
   and must preserve that network mode after exit.

## Keyboard focus recovery check

1. Enter team selection or gameplay through an online Host/Guest launch.
2. Switch to another application, release all keys, then click the game window
   or switch back. Confirm movement, Z, X, both Shift keys, and menu controls
   resume without restarting either peer.
3. While another application is foreground, pressing gameplay keys must not
   move the background game's keyboard-controlled player. On one PC with two
   game windows, only the foreground window may sample the shared keyboard.
4. Repeat several times, including minimizing/restoring the game and switching
   focus while holding a key. Check that released keys do not remain stuck.
5. If the original game's configuration disables DirectInput, check the same
   controls using the foreground fallback. Logs report
   `multi keyboard: foreground fallback` when this path becomes active.
6. Repeat a local two-device run separately. It must retain per-device Raw Input
   routing; the online fallback must not merge P1/P2 keyboards in local mode.

The keyboard tests emulate acquisition/read failures and focus races. They do
not inject real keyboard input or reproduce a particular user's driver failure.

## Security boundary

The launcher and game listen on all local IPv4 interfaces. The protocol has no
authentication or encryption, so only connect to a trusted peer. Restrict any
Windows firewall exception to the intended network and, when practical, the
other peer's address. See [`SECURITY.md`](../SECURITY.md).

## Input-delay starting points

| Network | Suggested delay |
| --- | ---: |
| Same PC or wired LAN | 1-2 |
| Typical LAN/Wi-Fi | 3 |
| Trusted VPN with moderate latency | 4-6 |
| Unstable trusted VPN test | 7-12 |

A larger value adds control latency but gives late input packets more time to
arrive. Both peers should enter the same value.

## Manual gameplay results

Record each observed result in [the validation
ledger](MULTIPLAYER_VALIDATION.md). For a failed item, record:

- Host/Guest build commit;
- both selected teams;
- stage and approximate game time;
- which player performed the action;
- expected and observed HUD values;
- whether both windows remained synchronized;
- the exact game-window error title and relevant `log.txt` files.

Do not add original executables, DAT archives, score files, or user logs to Git.

## Automated packet-safety checks

The normal VC7 test executable includes a deterministic malformed-packet
corpus. CI additionally runs the same production decoders under libFuzzer,
AddressSanitizer, and UndefinedBehaviorSanitizer:

```text
python scripts/run-multiplayer-protocol-fuzz.py --seconds 60
```

This command requires `clang++` with libFuzzer support. A crash input written
under `build/security/` must be treated as untrusted and must not contain game
data or unrelated packet captures.
