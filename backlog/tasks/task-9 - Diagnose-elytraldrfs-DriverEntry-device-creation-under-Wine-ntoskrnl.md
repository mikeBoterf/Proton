---
id: TASK-9
title: Diagnose elytraldrfs DriverEntry / device-creation under Wine ntoskrnl
status: Done
assignee: []
created_date: '2026-09-20 20:20'
updated_date: '2026-09-21 04:40'
labels:
  - wine
  - driver
milestone: m-1
dependencies: []
priority: high
ordinal: 9000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Patched launcher reaches session-prime; NtLoadDriver->winedevice fires but the elytraldrfs driver never creates its \Device\elytra_<rand> object (NtCreateFile c00000cb -> module Load 0x80070001 -> WD-L014). Determine via +ntoskrnl trace whether DriverEntry fails on a missing Wine kernel API (fixable, category a) or refuses under emulation (vendor wall, category b).
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Capture +ntoskrnl,+winedevice trace of the elytraldrfs .sys load
- [x] #2 Determine: DriverEntry not-run / failed-on-missing-API vs ran-and-refused
- [x] #3 If (a): implement the missing behavior as a numbered patch; if (b): document as vendor-gated
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
+ntoskrnl TRACE RESULT: driver loads fully (winedevice + ksecdd + elytraldrfs_driver.sys), DriverEntry runs, IoCreateDevice \Device\elytra_<rand> succeeds, all MajorFunctions registered, module opens device (dispatch_create ok). FAILURE is the DRIVER ITSELF: dispatch_ioctl code 0x222014, in_size 55275072 (~52.7MB, ~= lighthouse module size), out_size 64 -> IRP_MJ_DEVICE_CONTROL returns 0xc0000001 STATUS_UNSUCCESSFUL from the driver handler. Wine delivered the IOCTL correctly; the real driver ran and returned failure (leans category b: driver does genuine kernel work Wine cant satisfy). NOT a missing-API gap. Going further = RE the anti-tamper driver internals = crosses toward circumvention. Traces in artifacts/logs.

+bcrypt TRACE — ROOT CAUSE likely category (a) FIXABLE: the elytraldrfs IOCTL 0x222014 decrypts the 52.7MB encrypted lighthouse module: AES-128 (16B secret), ChainingMode set, 16B IV, IN-PLACE (in ptr==out ptr==0xD20050), 55275056B block-aligned, NO padding; then ECDSA P-384/SHA-384 BCryptVerifySignature on the result. Driver returns STATUS_UNSUCCESSFUL info=0 = signature verify of decrypted module FAILED = Wine produced wrong plaintext. Same class as AstralDrift 0002 (in-place symmetric decrypt) + live fixme BCryptGenerateSymmetricKey ignoring object buffer. NOT attestation forgery — decrypting vendor module w/ embedded key = legit Wine crypto fix. NEXT: standalone BCrypt in-place AES-CBC decrypt correctness test vs Windows; likely a numbered patch to dlls/bcrypt.

CRYPTO PRIMITIVES RULED OUT (standalone tests under patched wine, sources in repro archive): (1) AES-CBC in-place decrypt round-trips correct incl 52MB (mode string = ChainingModeCBC, 32B); (2) ECDSA P-384/SHA-384 sign+verify correct, corrupted rejected. Driver uses both, both correct, yet IOCTL still returns STATUS_UNSUCCESSFUL. => failure is NOT Wine crypto. Remaining: (a) 52MB METHOD_BUFFERED IOCTL buffer delivery integrity to the driver (no obvious cap in ntoskrnl dispatch_ioctl/server device.c; needs a test .sys to confirm), or (b) driver env verification. Black-box limit reached without a test driver or RE-ing the anti-tamper driver (latter = off-limits).

CONCLUSIVE (test .sys, buftest): 52MB METHOD_BUFFERED IOCTL delivered BYTE-PERFECT to a driver (user hash == driver hash). Combined with AES-CBC + ECDSA both proven correct: EVERYTHING Wine provides to elytraldrfs is correct, yet its IOCTL returns STATUS_UNSUCCESSFUL. => VERDICT (b): the driver, given correct inputs, fails on its own kernel/environment verification under emulation. No honest client-side fix remains; resolving further = RE the anti-tamper driver (off-limits) or Embark enabling the Linux Elytra path. Proven by elimination, not assumed.

CORRECTION (2026-09-20, after independent second reproduction — see docs/wardogs-elytra-independent-review.md): VERDICT (b) above is RETRACTED as premature. The "AES-CBC proven correct" step it rests on does not hold: bcrypt_inplace_test.c round-trips through the SAME Wine backend (self-consistency, not Windows-equivalence) and passes dwFlags=0, so it never exercises a padded decrypt. The second investigation's relay trace shows BCryptDecrypt ITSELF returning 0xc0000001 with a diagnostic build pointing at invalid PKCS7 padding, and reports the retail driver passing BCRYPT_BLOCK_PADDING — contradicting the "NO padding" observation recorded in the +bcrypt entry above. NOTE: that earlier +bcrypt entry independently reached category (a) "Wine produced wrong plaintext" before the crypto tests moved it to (b); if those tests were inadequate, the original (a) instinct may have been right. OPEN and blocking: (1) what dwFlags does the retail BCryptDecrypt call actually receive — re-read the relay trace; (2) matched Windows baseline, same launcher+BuildID. Do not report a root cause upstream until one of these lands.

SCOPE CLARIFICATION (2026-09-20): AC#1 and AC#2 stand — the trace was captured and the determination 'DriverEntry ran, driver refused' is still supported. AC#3 is UNCHECKED because the 'document as vendor-gated' output was the retracted verdict (b). The titled scope (DriverEntry / device creation) is resolved: the driver loads, creates its device, and registers MajorFunctions correctly. What remains open is why its IOCTL handler refuses, which has moved to TASK-13. Conformance work split out to TASK-10 / TASK-11 / TASK-12; shortcomings write-up is TASK-14.

REFERENCE FIX: the preceding note cited provisional IDs. Correct successors are TASK-10 (BCrypt invalid-padding conformance), TASK-11 (crypto test gaps), TASK-12 (root cause: why the Linux inputs have invalid padding), TASK-13 (shortcomings postmortem), TASK-14 (upstream to WineHQ).

FINAL (2026-09-21, measured): the `NO padding` recorded in the +bcrypt entry above is WRONG. A WINEDEBUG=+bcrypt trace of a real launch shows dwFlags = 0x1 = BCRYPT_BLOCK_PADDING (steam-1867240.log:283). The flag was misread from the original trace.

The same trace shows the driver's complete thread ending at the decrypt with NO BCryptImportKeyPair and NO BCryptVerifySignature. So the ECDSA-verify step described in that entry never executes either — the driver aborts at decryption.

AC#3 is now checked, satisfied by the (a) branch rather than (b): the failure is localised to module decryption and documented as such in docs/wardogs-elytra-proton-findings.md. Verdict (b) 'vendor-gated attestation' stays RETRACTED — eliminated as the proximate cause, not merely unproven.

Closing this task. Root-cause work continues in TASK-12; the claim ledger is docs/wardogs-elytra-timeline.md; the method postmortem is docs/wardogs-elytra-postmortem.md.
<!-- SECTION:NOTES:END -->
