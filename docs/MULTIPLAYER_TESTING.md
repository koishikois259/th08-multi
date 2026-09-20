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
