---
id: TASK-5
title: Implement real Windows volume-GUID path resolver in Wine
status: To Do
assignee: []
created_date: '2026-09-20 04:43'
updated_date: '2026-09-20 20:45'
labels:
  - wine
  - upstream
milestone: m-2
dependencies: []
priority: medium
ordinal: 5000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Resolve \\??\\Volume{GUID}\\... paths against each drive's volume serial (GUID low byte = drive letter), replacing the dosdevices-symlink workaround everyone currently uses. Our unclaimed upstreamable value-add. Develop and test against the packaged compat tool (depends on m-1).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Wine resolves \\??\\Volume{GUID}\\ paths natively (no dosdevices symlink)
- [ ] #2 Elytra module install succeeds with the resolver and no symlink present
- [ ] #3 Added as a numbered patch in nix/wine-patches/ with provenance + a test
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
Note: WD-L014 root cause is bcrypt in-place decrypt (see TASK-9), separate from this volume-GUID resolver. Both are category (a).
<!-- SECTION:NOTES:END -->
