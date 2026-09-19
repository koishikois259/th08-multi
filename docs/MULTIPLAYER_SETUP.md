# th08-multi trusted LAN/VPN setup

The multiplayer fork reads `th08_multi.ini` from the executable's working
directory. Copy `th08_multi.ini.example` to that name on both computers.

## Host

```ini
[network]
mode=host
bind_address=0.0.0.0
host_port=17708
local_port=17708
input_delay=3

```

The launcher writes `bind_address=0.0.0.0` automatically and displays the
computer's detected addresses in its `Local IPv4:` row. There is no Bind input
field. If a firewall rule is needed, restrict UDP 17708 to the intended
network and trusted peer.

## Guest

```ini
[network]
mode=guest
bind_address=0.0.0.0
host=192.168.1.20
host_port=17708
local_port=0
input_delay=3

```

The launcher manages `bind_address`; the Guest only enters the Host's address
on the network or tunnel used for the session. Keep `127.0.0.1` as the Host
address only for two-process testing on one PC. The negotiated delay is the
larger of the two requested values.

This protocol has no peer authentication or encryption. Use it only with a
person you trust on a private LAN or trusted VPN; public-Internet connections
and router port forwarding are outside the supported security boundary. See
[`SECURITY.md`](../SECURITY.md).

Start both launchers and select **Connect**. The Host waits on its configured
UDP port while the Guest performs a launcher handshake. The Host's **Start
both games** button is disabled until both launchers report `Connected`; the
Guest cannot start independently. When the Host selects **Start both games**,
the launchers exchange a start acknowledgement, release their rendezvous
sockets, and start both game processes. The games then establish their normal
lockstep session on the configured game port.

The v0.34 game executable keeps title-menu input locked until both peers have
reported ready. P1 (the host) controls shared title-menu and
difficulty choices. On the character-select screen P1 and P2 independently
move and confirm their own team cursor; identical teams are allowed and play
starts after both cursors are locked. Both players control their own characters
once gameplay starts. Either player can open or close the in-game pause menu.
The v0.34 multiplayer menu supports Story and Extra runs; Practice and Spell
Practice remain single-player-only.

The internal replay bookkeeping object remains present because retail enemy
logic depends on it, but multiplayer replay selection and saving are disabled
for v0.34.

Both players start with the Host/P1 initial-lives setting. The Guest/P2 local
setting is ignored for the multiplayer run; the unchanged default is two.
