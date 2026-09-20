---
id: TASK-3
title: Capture clean release-build (1867240) failure trace
status: Done
assignee:
  - '@mboterf'
created_date: '2026-09-20 04:43'
updated_date: '2026-09-20 20:31'
labels:
  - repro
  - docs
milestone: m-1
dependencies: []
priority: high
ordinal: 3000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Produce the reproducible artifact nobody has published: a PROTON_LOG + WINEDEBUG(+seh,+tid,+file,+eventlog) capture of the exact failure on the SHIPPING EA build 1867240 (not the 4809930 playtest), with our nix-pinned Proton. eventlog surfaces Elytra's own error.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 steam-1867240.log captured with the packaged tool
- [x] #2 Exact failure step + Elytra error code identified and written up
- [x] #3 Repro steps + nix rev pinned in backlog/docs
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
TRACE CAPTURED (patched launcher, no bypass, fresh prefix + C: volume symlink): launcher runs, modules install, service+session OK, NtLoadDriver->winedevice fires. Stops at elytraldrfs (23da072e@10) module Load: driver device \Device\elytra_<rand> never created -> NtCreateFile c00000cb -> IElytraModule::load 0x80070001 -> WD-L014. Unknown if DriverEntry failed on missing Wine kernel API (a, fixable) or detected emulation (b, wall). Needs +ntoskrnl trace. Full log saved to wardogs-elytra-proton/artifacts/logs.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Captured full PROTON_LOG + +ntoskrnl/+winedevice/+eventlog traces of the real launcher path on release build 1867240, isolating the exact failure: elytraldrfs IOCTL 0x222014 -> STATUS_UNSUCCESSFUL. Logs archived in wardogs-elytra-proton/artifacts/logs. Ready as the reproducible artifact for Valve issue #10113 / Embark.
<!-- SECTION:FINAL_SUMMARY:END -->
