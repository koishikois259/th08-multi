# th08-multi v0.21 manual network testing

`th08-multi-launcher.exe` writes `th08_multi.ini`, establishes the pre-game
connection, and starts `th08-multi.exe` on both peers after the Host issues the
start command. Keep both executables in the same directory as the legally
obtained TH08 1.00d data files.

The distributable `th08-multi-v0.21-patch.zip` contains no original game data.
Extract all of its files directly into a Japanese TH08 1.00d directory, then
start `th08-multi-launcher.exe`. The original `th08.exe` remains untouched.

## LAN test

On the Host PC:

1. Start `th08-multi-launcher.exe`.
2. Select **Host**.
3. Set Host UDP port and Local UDP port to the same value. The default is
   `17708`.
4. Use the displayed local IPv4 address on the Guest PC.
5. Choose **Connect** and wait for the status to report `Connected`.
6. After the Guest is connected, choose **Start both games**.

On the Guest PC:

1. Start the launcher and select **Guest**.
2. Enter the Host PC's LAN IPv4 address.
3. Enter the Host's UDP port. Local UDP port may remain `0` for automatic
   selection.
4. Set the same input delay as the Host; start with `3`.
5. Choose **Connect** and wait for the Host. The Guest start button remains
   disabled; the game starts when the Host chooses **Start both games**.

The launcher reports waiting, connecting, connected, and synchronized-start
states. After launch, the game window title reports listening, connecting,
connected, team-select, playing, and network-error states. Both computers must
use the same v0.21 build and the same Japanese TH08 1.00d game data.

## Internet test

The Host must forward the selected UDP port on the router to the Host PC's LAN
IPv4 address and allow the same UDP port through the local firewall. The Guest
enters the Host network's public IPv4 address. The launcher does not provide
relay, matchmaking, automatic port forwarding, or NAT traversal in v0.21.

Never expose file-sharing services or unrelated ports. Only the configured
game UDP port is required.

## Input-delay starting points

| Network | Suggested delay |
| --- | ---: |
| Same PC or wired LAN | 1-2 |
| Typical LAN/Wi-Fi | 3 |
| Internet with moderate latency | 4-6 |
| Unstable/high-latency test | 7-12 |

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
