---
id: TASK-18
title: Build 32-bit bcrypt.dll for compat tool parity
status: To Do
assignee: []
created_date: '2026-09-21 04:52'
labels:
  - packaging
  - bcrypt
milestone: m-1
dependencies: []
priority: low
type: chore
ordinal: 18000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
wardogs-elytra-v2 ships a patched 64-bit bcrypt.dll only. files/lib/wine/i386-windows/bcrypt.dll and the syswow64 prefix copy are still the unpatched v1 build (sha c19cbdcae0deccda).

Irrelevant to the current investigation — the launcher, the driver and the failing decrypt are all 64-bit — but it is a silent asymmetry in a tool being used to A/B behaviour, and asymmetries like this are exactly what produce confusing results later.

Needs a reconfigure with --enable-archs=i386,x86_64 and a rebuild of the i386 PE target.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 32-bit bcrypt.dll built from the same submodule state
- [ ] #2 Installed into both lib/wine/i386-windows and default_pfx syswow64
- [ ] #3 Tool notes updated to remove the KNOWN GAP paragraph
<!-- AC:END -->
