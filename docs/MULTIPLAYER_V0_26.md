# th08-multi v0.26 release checkpoint

v0.26 is a launcher usability update built on v0.25.

## Launcher

- Restored the earlier launcher layout: `Local IPv4:` is displayed as
  connection information and there is no `Bind local IPv4` input row.
- The launcher automatically listens on all local IPv4 interfaces and writes
  `bind_address=0.0.0.0` for the game process. This avoids requiring a player
  to select a LAN, VPN, or tunnel adapter manually.
- Because the socket listens on every local interface, only connect to a
  trusted peer and restrict any firewall rule to the intended network/peer.

## Compatibility

Both peers must use the same v0.26 patch. Gameplay and launcher fingerprints
are advanced to v0.26, preventing accidental mixed-version sessions with
v0.25. The supported base game remains Japanese TH08 1.00d.

## Local release artifact

`th08-multi-v0.26-patch.zip` was built and installed locally with SHA-256
`ccaff8925c2c94fb555f66cfc7fa64d2241bb357b9ed5091938b2089dd44e7ea`.
The archive contains no original executable, DAT archive, save, or replay data.
