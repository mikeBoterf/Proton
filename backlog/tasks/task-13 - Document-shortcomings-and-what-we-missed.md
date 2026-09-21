---
id: TASK-13
title: Document shortcomings and what we missed
status: In Progress
assignee: []
created_date: '2026-09-21 04:12'
labels:
  - docs
  - postmortem
milestone: m-1
dependencies: []
priority: high
type: docs
ordinal: 13000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
An honest account of where this investigation went wrong, written so the next person does not repeat it. Distinct from the findings docs: this is about method, not mechanism.

Material to cover:
- Argument from elimination was treated as proof. Verdict (b) in TASK-9 rested on "everything Wine provides is correct", which was never established to Windows-equivalent standards.
- The crypto tests self-round-tripped through the backend under test and never exercised the padded path, yet were cited as proof of bit-for-bit correctness.
- A test whose exit code was hardcoded to 0 was treated as passing.
- The findings doc's summary claimed the patched launcher reached a live match; that session came from the launcher-bypass path, which does not exercise these patches.
- An earlier +bcrypt note reached the decryption reading first, and it was abandoned on the strength of the inadequate tests. The record shows the correct instinct being argued away.
- What still has not been obtained: a native Windows kernel BCrypt/IOCTL trace. User-mode instrumentation and Procmon cannot supply it.
- What the second investigation caught that the first did not, and why a single-investigator loop missed it.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Written as method postmortem, not a restatement of the findings
- [ ] #2 Each claimed shortcoming cites the file and line that demonstrates it
- [ ] #3 States plainly what is still unknown
- [ ] #4 Linked from both findings documents
<!-- AC:END -->
