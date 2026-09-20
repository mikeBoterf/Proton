---
id: TASK-1
title: Package runnable steamcompattool from patched wine-proton
status: To Do
assignee: []
created_date: '2026-09-20 04:43'
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
- [ ] #3 Patched ntdll/ntoskrnl/bcrypt from wine-proton are the ones loaded at runtime (verified)
<!-- AC:END -->
