---
id: TASK-4
title: 'File reproducible repro on Valve Proton issue #10113'
status: To Do
assignee: []
created_date: '2026-09-20 04:43'
updated_date: '2026-09-20 21:35'
labels:
  - upstream
  - docs
milestone: m-1
dependencies: []
priority: medium
ordinal: 4000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Post the release-build (1867240) reproducible build + trace to ValveSoftware/Proton issue #10113, distinguishing it from the playtest reports and pointing at the vendor-gated attestation ceiling (Secure Boot/HVCI/DSE/non-VM). Deliverable, not a fix.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Comment drafted with nix rev, repro steps, and the exact failure
- [ ] #2 Posted (by the user) and linked in backlog/docs
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
Findings documented: docs/wardogs-elytra-proton-findings.md (full public write-up: what we built, exact WD-L014 mechanism, which kernel APIs hit Wine handlers + all-correct crypto/buffer proofs, how a Linux path would work if Embark enabled one, no-spoofing framing) + docs/elytra-diagnostics/ (the 4 standalone test sources). Ready to commit/push public.
<!-- SECTION:NOTES:END -->
