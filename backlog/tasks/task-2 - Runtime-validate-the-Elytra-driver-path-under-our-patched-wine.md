---
id: TASK-2
title: Runtime-validate the Elytra driver path under our patched wine
status: Done
assignee:
  - '@mboterf'
created_date: '2026-09-20 04:43'
updated_date: '2026-09-20 20:31'
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
- [x] #1 Elytra service + control.exe status OK under the packaged tool
- [x] #2 All 3 modules install and verify
- [x] #3 session prime reaches the lighthouse Load step (documented outcome/error code)
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
RUNTIME RESULT: patched Proton (wardogs-elytra) launches WARDOGS 1867240 to the game client + tutorial. No ElytraLauncher.last-error.json = launcher/session-prime SUCCEEDED (WD-L010/L017 wall cleared vs stock Proton). Confirmed our wine in proc tree. PROTON_LOG not set this run (no steam-1867240.log). Next: (a) test live multiplayer = heartbeat window; (b) relaunch with PROTON_LOG for the repro trace.

DEFINITIVE: full client path works under patched Proton — launcher/session-prime/driver-load/client/tutorial all succeed, JOINED A LIVE MATCH, played ~60s, then server kick "no valid heartbeat within window" at exactly 1:00. Further than any public build (AstralDrift reached firing range only). Confirms residual is the heartbeat window. OPEN: is heartbeat not-sent (fixable client gap) vs sent-but-rejected (vendor-gated)? Needs a PROTON_LOG+eventlog run to tell.

CORRECTION: that run had the launcher-BYPASS still in launch options (exec swaps WardogsLauncher->WardogsClient), so Elytra launcher/session-prime was SKIPPED, not exercised. The 60s heartbeat kick = stock bypass behavior, NOT a test of our patches. Prior "launcher succeeded" note was wrong (no last-error.json because launcher never ran). Re-test REQUIRED with bypass removed: launch options = PROTON_LOG=1 WINEDEBUG=+seh,+tid,+file,+eventlog %command% only, so WardogsLauncher runs on patched wine.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Validated end-to-end: patched Proton runs the real WARDOGS launcher (no bypass) through module install (needs C: volume-GUID symlink), service+session, NtLoadDriver->winedevice, full elytraldrfs DriverEntry + device create/open. Terminal failure is the driver's OWN IOCTL 0x222014 (52MB in) returning STATUS_UNSUCCESSFUL -- driver ran, chose to fail. Deepest legit-path characterization to date.
<!-- SECTION:FINAL_SUMMARY:END -->
