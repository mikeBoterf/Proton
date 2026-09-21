---
id: TASK-12
title: Find why the Linux BCryptDecrypt inputs have invalid padding
status: In Progress
assignee: []
created_date: '2026-09-21 04:12'
updated_date: '2026-09-21 04:47'
labels:
  - wine
  - bcrypt
  - root-cause
milestone: m-1
dependencies: []
priority: high
type: spike
ordinal: 12000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
The real open question behind WD-L014. Neither the status-code fix (TASK-10) nor the Windows baseline explains WHY the ciphertext reaching BCryptDecrypt on Linux fails padding validation.

Why the status fix almost certainly will not resolve it: on Windows the real decrypt SUCCEEDS and returns 0x00000000. Windows never reaches the code path whose status Wine reports wrongly. Correcting an error code for a failure Windows does not experience cannot make the Linux launch succeed, unless the driver has a status-sensitive retry branch.

Highest-value experiment (LINUX-NEXT-STEPS.md step 4, reprioritized to first): within a SINGLE Linux run, compare lengths and content digests at the service buffer, the IOCTL dispatch entry, and the BCrypt input immediately before decryption, accounting for the documented 16-byte offset. A boundary mismatch narrows it to an upstream copy/offset/lifetime bug. Matching boundaries redirects toward input construction or earlier API behaviour.

Also still unanswered: what dwFlags the retail driver actually passes to BCryptDecrypt. The two findings docs contradict each other on this and the Windows package could not settle it (no native kernel BCrypt/IOCTL trace was obtainable with user-mode tooling).

Constraints: do not log keys, raw plaintext, full ciphertext, or session tokens. Do not bypass the launcher or alter anti-cheat decisions.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Retail BCryptDecrypt dwFlags determined from the existing relay trace, or recorded as unobtainable
- [ ] #2 Boundary digests compared at service buffer / IOCTL entry / BCrypt input in one run
- [ ] #3 Result states whether boundaries match, without logging sensitive material
- [ ] #4 Conclusion recorded as established or still open — no argument from elimination
- [x] #5 CAB inputs compared against the Windows-measured hashes
- [x] #6 Wine's padded in-place decrypt tested at retail size against externally generated ciphertext
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
2026-09-21 ~23:33 local: v2 RUN RESULT — WD-L014 PERSISTS.

Error shown: WD-L014-0853961767dd (suffix is a per-session correlation id; the earlier run showed WD-L014-7538de5103e6 — not a meaningful difference).

Run is INTERPRETABLE, verified before drawing any conclusion:
- Steam CompatToolMapping for 1867240 = `wardogs-elytra-proton-v2`
- prefix drive_c/windows/system32/bcrypt.dll = 01bf6af5045a47ed (the PATCHED build), timestamped 23:33:05 i.e. this run. Proton refreshed the prefix copy, so the fix was genuinely loaded.
- syswow64/bcrypt.dll = c19cbdcae0deccda, still the unpatched 32-bit build (known gap; launcher and driver are 64-bit).

WHAT THIS ESTABLISHES: the TASK-10 conformance fix (STATUS_DATA_ERROR + IV advance + pcbResult) does NOT change the launcher outcome. This disfavors the status-sensitive-caller hypothesis — the one remaining way the status fix could plausibly have mattered. Predicted outcome; on Windows the retail decrypt returns 0x00000000, so Windows never reaches the corrected branch at all.

WHAT IT DOES NOT ESTABLISH: where the run failed. No trace was captured (PROTON_LOG was not set), so we cannot say whether BCryptDecrypt still fails on padding, now fails with the corrected status, or whether the failure has moved. A negative without a trace only rules out the status hypothesis; it does not advance the root cause.

NEXT — this is how AC#1 gets answered. dlls/bcrypt/bcrypt_main.c:2462 TRACEs BCryptDecrypt's full parameter list, with dwFlags as the final %#lx field. A `WINEDEBUG=+bcrypt` run therefore prints the retail driver's flags directly, settling the BCRYPT_BLOCK_PADDING contradiction between the two findings docs without any new instrumentation. Capture with PROTON_LOG=1 WINEDEBUG=+bcrypt in Steam launch options and locate the call with input_len 55275056.

2026-09-21: AC#1 ANSWERED, MEASURED. PROTON_LOG=1 WINEDEBUG=+bcrypt on a v2 run (~/steam-1867240.log:283).

  BCryptDecrypt 0xC221E0, 0xD20050, 55275056, (null), 0xC1F820, 16, 0xD20050, 55275056, 0xC1F75C, 0x1

Last field is dwFlags = 0x1 = BCRYPT_BLOCK_PADDING. THE RETAIL DRIVER DOES PASS PADDING.

Resolves the contradiction: docs/wardogs-elytra-independent-review.md is CORRECT. The claim of 'no padding' in docs/wardogs-elytra-proton-findings.md and in the +bcrypt note on TASK-9 is WRONG and has been corrected.

Also confirmed from the same line: in-place (input ptr == output ptr == 0xD20050), 16-byte IV, input_len == output_len == 55,275,056 (block-aligned, 3,454,691 blocks).

SECOND AND LARGER RESULT — the complete driver-thread (01e8) sequence is:
  BCryptOpenAlgorithmProvider L"AES" / BCryptSetProperty L"ChainingMode" (32B) /
  BCryptGetProperty L"ObjectLength" / BCryptGenerateSymmetricKey (secret 16B,
  object buffer 654B -> 'fixme: ignoring object buffer') / BCryptDecrypt (above) /
  BCryptDestroyKey / BCryptCloseAlgorithmProvider

There is NO BCryptImportKeyPair and NO BCryptVerifySignature on that thread. The driver tears down immediately after the decrypt and NtUnloadDriver follows. It never reaches signature verification, and therefore never reaches anything that could perform environment attestation.

This eliminates reading A as the PROXIMATE cause of WD-L014. Whatever lighthouse may or may not check, the failure occurs strictly before it.

NOT YET KNOWN: why the decrypt fails. BCryptDecrypt TRACEs entry only, so the return status is not in this log. The absence of downstream verify calls is strong structural evidence it failed, but it is inference, not a logged status.

RULED OUT AS NOISE: `warn:bcrypt:get_gnutls_cipher handle block size` appears next to the decrypt but is an unconditional upstream reminder at gnutls.c:833 that fires for every AES key. Not an error.

OPEN LEAD: `BCryptGenerateSymmetricKey ignoring object buffer` (bcrypt_main.c FIXME). The driver queries ObjectLength and supplies a 654-byte key object; Wine ignores it and allocates internally. On Windows the key object lives in the caller's buffer. Not obviously causal for a single generate/decrypt/destroy cycle, but it is a real divergence on the exact call sequence that fails, and it was flagged in the original +bcrypt note too.

2026-09-21, TWO ELIMINATIONS BY DIRECT MEASUREMENT.

1. CAB INPUTS ARE IDENTICAL TO WINDOWS. Compared all three cached CABs in the game prefix (drive_c/Program Files/Elytra/Content) against the Windows-measured hashes in the windows-results package (protected-hashes-admin.json). All three match on both size and SHA-256:
     04207e7ef19c   602123 bytes    MATCH
     0420a0b6d89c   189935 bytes    MATCH
     0420eb484e3f 55329835 bytes    MATCH
   This is the comparison LINUX-NEXT-STEPS.md asked for and that nobody had run. Rules out 'Linux fetched a different or corrupted module'.

2. WINE'S CRYPTO IS CORRECT AT THE RETAIL SIZE AND SHAPE. New test docs/elytra-diagnostics/large-padded-test.c closes the last gap in the suite: the 52.7MB case in bcrypt_inplace_test.c self-round-trips through Wine, and the openssl fixtures were only 48 bytes, so nothing had tested retail size against external ciphertext. Generated 55,275,056 bytes with openssl (55,275,055 pattern bytes + one 0x01 PKCS7 byte) and decrypted in-place under v2 with BCRYPT_BLOCK_PADDING:
     status = 0x0, pcbResult = 55275055, mismatched plaintext bytes = 0 of 55275055,
     IV advanced correctly. RESULT: PASS.
   Incidental: Wine reports ObjectLength = 654, exactly the size the retail driver allocates for its key object. No ObjectLength mismatch.

CONSEQUENCE — the crypto is exonerated. Wine performs this exact operation, at this exact size and layout, bit-correctly. Therefore if the driver's decrypt fails, what reaches BCryptDecrypt (ciphertext, key or IV) must differ from what should reach it.

The remaining suspect surface is the DATA PATH, not the cipher:
   CAB 55,329,835 -> [extract/decompress] -> IOCTL buffer 55,275,072 -> [strip 16B IV] -> BCrypt input 55,275,056
The CAB end is now verified identical; the cipher end is now verified correct. The bug is between them.

STILL INFERRED, NOT OBSERVED: that the driver's BCryptDecrypt actually returns a failure on THIS machine. +bcrypt traces entry only. The inference is much stronger now — correct inputs demonstrably succeed — but a logged status would close it. Cheapest route: add a targeted TRACE of the padding-validation outcome to our own patched build (we control it), logging the observed padding length and resulting status, no plaintext.
<!-- SECTION:NOTES:END -->
