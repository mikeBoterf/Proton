---
id: TASK-7
title: Nix flake-parts build for patched Proton wine
status: Done
assignee: []
created_date: '2026-09-20 04:43'
updated_date: '2026-09-20 04:43'
labels:
  - nix
milestone: m-0
dependencies: []
priority: high
ordinal: 7000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Additive dendritic flake-parts layer: devshell + wine-proton derivation building Valve wine (proton_11.0 @ dc26e6184) with the Elytra patch series, reproducibly and offline.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 flake evaluates; nix flake show lists devShells+packages
- [x] #2 devshell provides mingw x64+x86 + autotools toolchain
- [x] #3 nix build .#wine-proton produces a dist with patches present in binaries
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Built a 706MB wine dist; verified RtlStringFromGUIDEx exported, real NtLoadDriver (SCM->winedevice) in ntdll.dll, bcrypt + MmMapLockedPagesSpecifyCache present. Reconstructed the git-wine bootstrap (make_requests, make_specfiles, make_vulkan -x/-X, autoconf, autoheader) + --without-ffmpeg; documented in nix/wine-patches/README.md and AGENTS.md.
<!-- SECTION:FINAL_SUMMARY:END -->
