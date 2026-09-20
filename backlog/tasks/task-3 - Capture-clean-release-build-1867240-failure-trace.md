---
id: TASK-3
title: Capture clean release-build (1867240) failure trace
status: To Do
assignee: []
created_date: '2026-09-20 04:43'
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
- [ ] #1 steam-1867240.log captured with the packaged tool
- [ ] #2 Exact failure step + Elytra error code identified and written up
- [ ] #3 Repro steps + nix rev pinned in backlog/docs
<!-- AC:END -->
