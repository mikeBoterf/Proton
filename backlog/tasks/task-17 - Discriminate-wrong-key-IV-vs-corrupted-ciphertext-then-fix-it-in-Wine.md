---
id: TASK-17
title: 'Discriminate wrong key/IV vs corrupted ciphertext, then fix it in Wine'
status: To Do
assignee: []
created_date: '2026-09-21 04:52'
labels:
  - wine
  - root-cause
  - fix
milestone: m-1
dependencies:
  - TASK-15
  - TASK-16
priority: high
type: bug
ordinal: 17000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
The fix task. By this point TASK-15 has the observed failure status and TASK-16 has said whether the ciphertext arriving at the driver matches the shipped module.

BRANCH A — ciphertext DIFFERS (TASK-16 says DIFFER).
The module is being corrupted between the CAB and the IOCTL. Suspects, in order: Wine's cabinet/extraction path, file I/O on a 52.7 MB read, the usermode->IOCTL copy, or MmMapLockedPagesSpecifyCache (patch 0003, which is ours and comparatively young). The first-divergence offset from TASK-16 discriminates: divergence at a page or buffer boundary points at mapping/copy; divergence from byte 0 points at extraction.

BRANCH B — ciphertext MATCHES.
The bytes are right, so the key or the IV is wrong. The 16-byte secret handed to BCryptGenerateSymmetricKey comes from somewhere; the 16-byte IV is presumed to be the leading 16 bytes stripped off the IOCTL buffer. Either could be mis-derived. Note the live FIXME on this exact sequence: `BCryptGenerateSymmetricKey ignoring object buffer` — the driver supplies a 654-byte key object that Wine discards for its own allocation. Flagged three times, never examined.

  Sub-branch B2 to keep in mind: if the key turns out to be derived from machine or
  session state that Wine reports differently, that is a different kind of problem and
  may edge back toward environment-dependence. Recognise it early rather than
  rediscovering it late.

USEFUL DISCRIMINATOR, IN BOUNDS: CBC error propagation. A corrupted ciphertext block damages only that block's plaintext plus the next, then self-heals; a wrong key or IV garbles everything (a wrong IV garbles the first block only). This can be reasoned about from CIPHERTEXT digests at block boundaries without ever inspecting plaintext.

SCOPE REMINDER: this remains compatibility work. Fix Wine so the real driver gets what it would get on Windows. Do not weaken validation, synthesize a result, or make a failing check pass.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Branch determined from TASK-15 and TASK-16 evidence, stated explicitly
- [ ] #2 Root cause identified and named — a specific Wine behaviour, not a category
- [ ] #3 Fix implemented as a numbered patch with an in-tree regression test
- [ ] #4 Fix verified by a real launch, not only by unit test
- [ ] #5 If the cause turns out not to be Wine's, say so plainly and stop
<!-- AC:END -->
