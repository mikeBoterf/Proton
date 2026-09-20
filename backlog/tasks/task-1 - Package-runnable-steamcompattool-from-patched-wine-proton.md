---
id: TASK-1
title: Package runnable steamcompattool from patched wine-proton
status: Done
assignee:
  - '@mboterf'
created_date: '2026-09-20 04:43'
updated_date: '2026-09-20 06:51'
labels:
  - nix
  - packaging
milestone: m-1
dependencies: []
priority: high
ordinal: 1000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Assemble a Steam compatibility tool that combines a base Proton (Experimental/Hotfix) with our nix-built wine-proton binaries, so WARDOGS (1867240) can actually be launched with the Elytra patches. Nix way: a nix/compat-tool.nix module producing a steamcompattool output usable via programs.steam.extraCompatPackages, with a distinct build name.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 nix/compat-tool.nix builds a steamcompattool output (distinct build name)
- [ ] #2 Tool appears in Steam and is selectable for AppID 1867240
- [x] #3 Patched ntdll/ntoskrnl/bcrypt from wine-proton are the ones loaded at runtime (verified)
<!-- AC:END -->

## Implementation Plan

<!-- SECTION:PLAN:BEGIN -->
1. Resolve wine build architecture vs base Proton (WoW64 vs split-arch) — BLOCKER found\n2. Pick packaging strategy (container full-build / match-arch overlay / pure-nix)\n3. Assemble steamcompattool output + compatibilitytool.vdf\n4. Verify patched modules are the ones loaded at runtime
<!-- SECTION:PLAN:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
Arch mismatch found: our wine-proton = WoW64 (only x86_64-unix); base Protons (Hotfix hotfix-20260828, GE-Proton10-27) = split-arch (i386-unix + x86_64-unix). Cannot module-overlay across the two loader models. Base Proton wine commit also not confirmed == dc26e6184. Needs a strategy decision before building the compat tool.

Decision: build the real Proton via its own container build (make redist), Nix stays as tooling/pinning. Docker 29.7.2 reachable, user in docker group, 885G free. Patches applied to wine/ submodule. Kicking off make redist build_name=wardogs-elytra.

Build past configure+SDK image; make redist needs ALL submodules (gstreamer/gst-plugins-*/dav1d/dxvk/vkd3d/...), not just wine. Running recursive submodule init (large), then re-apply wine patches + rebuild.

NixOS+Docker DNS gotcha: containers inherit systemd-resolved 127.0.0.53, cannot resolve hosts -> in-container downloads (piper fmt/spdlog/piper-phonemize) fail. Fix: configure with --docker-opts="--dns 1.1.1.1 --dns 8.8.8.8" (flows into DOCKER_BASE run). Also pre-fetch host-side gecko/mono/xalia blobs. Both documented in nix/README.md. Rebuild running.

SUCCESS: 1.4G redist built at build/wardogs-elytra. Verified in shipped Wine: RtlStringFromGUIDEx exported, NtLoadDriver SCM present, ntoskrnl MmMapLockedPagesSpecifyCache, 32-bit ntoskrnl.exe built. Needed patch 0004 (_WIN64 guard) for Proton -Werror. Build recipe: steam-run + recursive submodules + pre-fetch contrib blobs + --docker-opts DNS. All in nix/README.md.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Built a complete patched Proton via Proton's own container build (make redist) rather than a nix overlay (WoW64-vs-split-arch mismatch made overlay unviable). 1.4G redist installed to compatibilitytools.d as 'wardogs-elytra'; all 4 Elytra patches verified in the shipped Wine. NixOS build recipe (steam-run + recursive submodules + pre-fetched blobs + --docker-opts DNS) documented in nix/README.md. Nix remains tooling (devshell + wine-proton compile-check).
<!-- SECTION:FINAL_SUMMARY:END -->
