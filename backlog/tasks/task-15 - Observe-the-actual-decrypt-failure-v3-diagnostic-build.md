---
id: TASK-15
title: Observe the actual decrypt failure (v3 diagnostic build)
status: To Do
assignee: []
created_date: '2026-09-21 04:51'
labels:
  - wine
  - bcrypt
  - root-cause
milestone: m-1
dependencies: []
priority: high
type: spike
ordinal: 15000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Close the last inference gap. We have never directly observed BCryptDecrypt failing on this machine — `+bcrypt` traces entry only, so the failure is inferred from the driver aborting with no signature verification after it. The inference is strong (TASK-12 proved correct inputs succeed at this exact size and shape) but it is still inference, and this investigation has already been burned once by reasoning where it could have measured.

We control the build. Add a targeted TRACE to the padding-validation branch in dlls/bcrypt/bcrypt_main.c logging the observed padding length and the resulting status. Rebuild bcrypt.dll, ship as compat tool v3, launch, read the log.

SCOPE: log the padding length byte and status only. No key, no IV, no ciphertext, no plaintext beyond the single structural padding byte that Wine already inspects to do its job. Do not dump or examine Elytra's decrypted module.

DISCRIMINATOR — what the padding length tells us:
- A wild/arbitrary value => the plaintext is garbage => wrong key, wrong IV, or corrupted ciphertext. Proceed to TASK-16/17.
- A plausible value (1..16) that simply fails the consistency check => something subtler; the final block is close but not right.
- STATUS_SUCCESS => the decrypt is NOT the failure and the whole current model is wrong. Stop and re-derive.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Targeted TRACE added to the padding-validation branch, logging padding length and status only
- [ ] #2 bcrypt.dll rebuilt and packaged as wardogs-elytra-v3
- [ ] #3 Launch captured with PROTON_LOG=1 WINEDEBUG=+bcrypt
- [ ] #4 Actual return status of the retail BCryptDecrypt recorded — observed, not inferred
- [ ] #5 Trace excerpt committed to docs/elytra-diagnostics/results/
- [ ] #6 Diagnostic TRACE kept out of the upstream patch 0005
<!-- AC:END -->
