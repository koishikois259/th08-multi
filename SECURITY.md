# Security policy

## Supported scope

Only the latest `th08-multi` release is supported. Multiplayer is intended
solely for two people who trust each other on the same private LAN or through
a trusted VPN. It is not designed or tested for direct public-Internet use.

The UDP protocol does not encrypt traffic, authenticate a peer's identity, or
protect against a malicious peer. The v0.26 launcher intentionally binds to
`0.0.0.0` so LAN, VPN, and tunnel adapters work without manual adapter
selection. Consequently, restrict any firewall exception to the intended
network and trusted peer addresses, and never connect to an untrusted peer.

## Reporting a vulnerability

Please do not open a public issue for a suspected vulnerability. Use GitHub's
private vulnerability reporting / Security Advisory flow for this repository
and include the affected commit, reproduction steps, impact, and any crash
input or packet capture. Do not include copyrighted TH08 game data, save files,
or unrelated personal data.

Security reports receive acknowledgement when the maintainer next checks the
repository. There is currently no paid bounty program or guaranteed response
deadline.

## Release checks

The repository includes deterministic malformed-packet tests and a libFuzzer
entry point compiled with AddressSanitizer and UndefinedBehaviorSanitizer in
CI. Runtime packet processing is capped at 64 gameplay datagrams per frame and
32 launcher datagrams per timer tick, so an input burst cannot monopolize one
update indefinitely. Malformed packets are dropped without per-packet logging;
the original game's accumulated `log.txt` buffer is fixed at 8 KiB. Patch
packaging rejects original game data and emits both an internal payload
checksum manifest and an external checksum for the ZIP archive.
