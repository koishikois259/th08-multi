# th08-multi v0.22 local/network setup

The multiplayer fork reads `th08_multi.ini` from the executable's working
directory. Copy `th08_multi.ini.example` to that name on both computers.

## Host

```ini
[network]
mode=host
host_port=17708
local_port=17708
input_delay=3

```

Allow inbound UDP port 17708 in the host firewall and forward the same UDP
port when connecting through a router. Direct IPv4 connections are the v0.22
scope; relay and automatic NAT traversal are not included.

## Guest

```ini
[network]
mode=guest
host=192.0.2.10
host_port=17708
local_port=0
input_delay=3

```

Replace `host` with the host's LAN or public IPv4 address. The negotiated delay
is the larger of the two requested values.

Start both launchers and select **Connect**. The Host waits on its configured
UDP port while the Guest performs a launcher handshake. The Host's **Start
both games** button is disabled until both launchers report `Connected`; the
Guest cannot start independently. When the Host selects **Start both games**,
the launchers exchange a start acknowledgement, release their rendezvous
sockets, and start both game processes. The games then establish their normal
lockstep session on the configured game port.

Wait for both game window titles to report `connected` before operating the
title menu. P1 (the host) controls shared title-menu and
difficulty choices. On the character-select screen P1 and P2 independently
move and confirm their own team cursor; identical teams are allowed and play
starts after both cursors are locked. Both players control their own characters
once gameplay starts. Either player can open or close the in-game pause menu.
The v0.22 multiplayer menu supports Story and Extra runs; Practice and Spell
Practice remain single-player-only.

The internal replay bookkeeping object remains present because retail enemy
logic depends on it, but multiplayer replay selection and saving are disabled
for v0.22.
