---
id: doc-1
title: Project status snapshot
type: readme
created_date: '2026-09-20 04:44'
---

## Goal

Let WARDOGS (Steam AppID `1867240`) run under Proton on Linux by completing the
Windows kernel behavior Elytra's real driver needs — **compatibility work only,
never anti-cheat/attestation forgery** (see AGENTS.md §2, §8).

## The ceiling (why "playable online" is vendor-gated)

Embark's own THE FINALS FAQs show the lighthouse kernel driver attests to
**Driver Signature Enforcement + Secure Boot + Memory Integrity (HVCI) + not-a-VM**
(VMs explicitly rejected). None of these exist in Wine's emulated `ntoskrnl`. So a
truthful driver's heartbeat is rejected **by design**; making it pass = forgery.
The Linux-playable path is Embark enabling Elytra's Proton attestation for WARDOGS
(as they did for THE FINALS / Arc Raiders), not anything we do client-side.

## Where we are (2026-09-20)

- **m-0 done:** `~/repos/Proton` fork (branch `proton_11.0`) has an additive
  dendritic flake-parts Nix layer. `nix build .#wine-proton` reproducibly builds
  Valve wine (`dc26e6184`) + the 3 Elytra patches; verified present in the dist
  (`RtlStringFromGUIDEx`, real `NtLoadDriver`, bcrypt fix, `MmMapLockedPagesSpecifyCache`).
- **Build bootstrap** (git-wine strips generated files): `make_requests`,
  `make_specfiles`, `make_vulkan -x vk.xml -X video.xml` (offline, writable HOME),
  `autoconf`, `autoheader`, `--without-ffmpeg`. In `nix/wine-patches/README.md`.
- **wine-proton-src input == wine submodule commit** (`dc26e6184`) — keep equal.

## Next (m-1, active)

1. TASK-1 package a `steamcompattool` (base Proton + our wine-proton).
2. TASK-2 runtime-validate the driver path actually reaches lighthouse prime.
3. TASK-3 capture the clean release-build (`1867240`) failure trace.
4. TASK-4 file the reproducible repro on ValveSoftware/Proton issue #10113.

Then m-2: real `\??\Volume{GUID}` resolver (our upstreamable value-add).

## Prior art

AstralDrift/Proton-WARDOGS-Elytra (our vendored patches; same wall).
JakubSzark heartbeat-only trick — dead (server now requires the kernel scanner),
ban-risky, do not revive.
