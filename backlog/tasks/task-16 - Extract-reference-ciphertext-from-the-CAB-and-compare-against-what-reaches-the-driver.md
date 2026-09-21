---
id: TASK-16
title: >-
  Extract reference ciphertext from the CAB and compare against what reaches the
  driver
status: To Do
assignee: []
created_date: '2026-09-21 04:51'
labels:
  - wine
  - root-cause
  - data-path
milestone: m-1
dependencies: []
priority: high
type: spike
ordinal: 16000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
TASK-12 verified both ends of the pipeline and found them sound:

  CAB 55,329,835 -> [extract/decompress] -> IOCTL 55,275,072 -> [strip 16B IV] -> BCrypt 55,275,056
       VERIFIED identical to Windows                                                VERIFIED correct

The bug is between them. This task localises it.

The CAB is a plain cabinet container and its payload stays encrypted throughout — extracting it is an integrity check, not circumvention. Extract lighthouse_driver.sys from 0420EB484E3F....cab with a Linux cabinet tool (cabextract / libmspack), independently of Wine. That yields the reference ciphertext as shipped.

Then compare, by digest only:
1. Reference extracted on Linux natively vs the size the IOCTL carries (55,275,072) and the size BCrypt receives (55,275,056). Account for the documented 16-byte offset — the leading 16 bytes are presumed to be the IV.
2. Digest of the reference vs digest of the buffer actually arriving at the driver.

If they differ, the extraction/transfer path under Wine is corrupting the module and we have a concrete Wine bug to chase. If they match, the ciphertext is fine and suspicion moves to the key or IV (TASK-17).

SCOPE: digests and lengths of CIPHERTEXT only. Never log or inspect decrypted plaintext. This keeps the no-reverse-engineering line intact — we are checking that bytes survive a copy, not looking at what they mean.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 lighthouse_driver.sys extracted from the CAB natively on Linux, outside Wine
- [ ] #2 Reference ciphertext size reconciled with 55,275,072 and 55,275,056, including the 16-byte offset
- [ ] #3 Digest of the reference compared against the buffer reaching the driver
- [ ] #4 Result states MATCH or DIFFER with the offset of first divergence if any
- [ ] #5 No plaintext logged or inspected at any point
<!-- AC:END -->
