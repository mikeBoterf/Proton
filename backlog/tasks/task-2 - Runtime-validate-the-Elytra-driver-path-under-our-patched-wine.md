---
id: TASK-2
title: Runtime-validate the Elytra driver path under our patched wine
status: To Do
assignee: []
created_date: '2026-09-20 04:43'
labels:
  - validation
milestone: m-1
dependencies: []
priority: high
ordinal: 2000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Confirm our nix-built patched wine behaves at runtime (not just symbol-present): Elytra service starts, modules install/verify, session create+configure+prime run, and the driver Load actually executes via winedevice. Establishes that our build reproduces AstralDrift's reach (lighthouse session-prime) on our target.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Elytra service + control.exe status OK under the packaged tool
- [ ] #2 All 3 modules install and verify
- [ ] #3 session prime reaches the lighthouse Load step (documented outcome/error code)
<!-- AC:END -->
