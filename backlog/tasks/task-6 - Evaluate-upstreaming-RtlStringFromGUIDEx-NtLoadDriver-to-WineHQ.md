---
id: TASK-6
title: Evaluate upstreaming RtlStringFromGUIDEx + NtLoadDriver to WineHQ
status: To Do
assignee: []
created_date: '2026-09-20 04:43'
labels:
  - wine
  - upstream
milestone: m-2
dependencies: []
priority: low
ordinal: 6000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Assess which vendored (AstralDrift) fixes are clean enough to submit to WineHQ independently of Elytra: RtlStringFromGUIDEx (trivial, clearly correct) and the NtLoadDriver->SCM routing. Coordinate provenance/authorship with AstralDrift before submitting.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Each candidate patch assessed for upstream-readiness (tests, correctness)
- [ ] #2 Authorship/provenance with AstralDrift resolved before any submission
<!-- AC:END -->
