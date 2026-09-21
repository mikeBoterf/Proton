---
id: TASK-14
title: Upstream the BCrypt invalid-padding conformance fix to WineHQ
status: To Do
assignee: []
created_date: '2026-09-21 04:13'
labels:
  - upstream
  - wine
  - bcrypt
milestone: m-1
dependencies:
  - TASK-10
priority: medium
type: task
ordinal: 14000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
The invalid-padding fix from TASK-10 stands on its own merits, independent of WARDOGS or any anti-cheat question. It resolves a literal `FIXME: invalid padding` that upstream Wine has carried on this branch, and it is backed by a measured Windows baseline plus an in-tree regression test.

Submit it on the conformance argument alone. Do NOT frame it as a WARDOGS or anti-cheat fix: it very likely does not fix WD-L014 (see TASK-12), the framing would be inaccurate, and it would make the patch harder to accept. The Windows measurements and the test are the whole case.

Carry the caveat that the failure-side pcbResult value is an observation from one Windows build rather than a documented contract, and be prepared to drop that part of the change if upstream objects to it.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Patch submitted to wine-devel with the Windows measurement as justification
- [ ] #2 Framed as a conformance fix, with no anti-cheat or WARDOGS framing
- [ ] #3 In-tree test included
- [ ] #4 pcbResult change flagged as observation-based and separable
<!-- AC:END -->
