---
id: TASK-4
title: 'File reproducible repro on Valve Proton issue #10113'
status: To Do
assignee: []
created_date: '2026-09-20 04:43'
updated_date: '2026-09-21 04:40'
labels:
  - upstream
  - docs
milestone: m-1
dependencies:
  - TASK-12
priority: medium
ordinal: 4000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Post the release-build (1867240) reproducible build + trace to ValveSoftware/Proton #10113 and #10163. BLOCKED on TASK-12: do not post a root-cause claim. The earlier framing (vendor-gated attestation ceiling) was retracted — see the TASK-9 correction. Any comment must present the failure as an open question with both candidate readings, not an established cause.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Comment states the measured facts only: padded in-place BCryptDecrypt (dwFlags=0x1) of 55,275,056 bytes, driver aborts with no signature verification after it
- [ ] #2 Explicitly does NOT claim a root cause for why the decrypt fails
- [ ] #3 Does not assert kernel attestation — eliminated as the proximate cause
- [ ] #4 Nix rev + repro steps + the +bcrypt capture recipe included
- [ ] #5 Posted (by the user) and linked in backlog/docs
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
Findings documented: docs/wardogs-elytra-proton-findings.md (full public write-up: what we built, exact WD-L014 mechanism, which kernel APIs hit Wine handlers + all-correct crypto/buffer proofs, how a Linux path would work if Embark enabled one, no-spoofing framing) + docs/elytra-diagnostics/ (the 4 standalone test sources). Ready to commit/push public.

2026-09-20: Blocked. The findings doc this task pointed at asserted a kernel-authenticity root cause that has since been retracted (see docs/wardogs-elytra-independent-review.md and the TASK-9 correction). Posting the previous draft would have put a claim on Valve's tracker that our own repo contradicts. Docs reworded to hold both readings open; this task waits for TASK-12 or ships as an explicit open question.

2026-09-21: Partially unblocked. The trace gives a precise, defensible statement of WHERE the failure is, which is worth posting even without a root cause: the driver makes one padded in-place BCryptDecrypt of 55,275,056 bytes and tears down immediately after, performing no signature verification. That is a much stronger and much smaller claim than either earlier draft.

Still blocked on TASK-12 for any statement about WHY. Do not repeat the attestation framing — it is now eliminated as the proximate cause, not merely unproven.
<!-- SECTION:NOTES:END -->
