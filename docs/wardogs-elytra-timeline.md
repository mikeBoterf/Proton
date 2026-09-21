# WARDOGS / Elytra — chronology of claims

A dated record of what was believed, on what evidence, and how each claim resolved.
Kept because the investigation reversed itself twice, and the reversals are more
instructive than the conclusions.

Companion documents:
[findings](wardogs-elytra-proton-findings.md) (what is true now) ·
[independent review](wardogs-elytra-independent-review.md) (second reproduction) ·
[postmortem](wardogs-elytra-postmortem.md) (why the wrong answer survived).

---

## Claim ledger

| # | Claim | Evidence at the time | Fate |
|---|---|---|---|
| 1 | Driver fails on a missing Wine kernel API | `+ntoskrnl` trace | **Wrong.** Driver loads, creates its device, registers dispatch routines, reaches its own logic. |
| 2 | Failure is the driver's own IOCTL handler | `+ntoskrnl`: `0x222014`, in 55,275,072, out 64, returns `0xc0000001` | **Holds.** |
| 3 | Root cause is decryption — "Wine produced wrong plaintext" | `+bcrypt` trace of the AES/ECDSA sequence | **Abandoned** on claim 5 — then **vindicated** by claim 9. |
| 4 | Driver calls `BCryptDecrypt` with **no padding** | Read from the same `+bcrypt` trace | **Wrong.** Contradicted by claim 9. The flag was misread. |
| 5 | Wine's AES and ECDSA are bit-for-bit correct | `bcrypt_inplace_test.c`, `ecdsa_test.c` | **Unsupportable.** Both self-round-trip through the backend under test; the AES test never passed `BCRYPT_BLOCK_PADDING` and returned 0 unconditionally. |
| 6 | IOCTL buffer delivered byte-perfect | `buftest.sys` — hash computed independently each side | **Holds.** The strongest test of the original set. |
| 7 | Therefore: residue is a direct kernel-authenticity check by `lighthouse` | Elimination, resting on claims 5 and 6 | **Retracted twice** — first as premature (claim 8), then eliminated outright (claim 10). |
| 8 | Claim 7 is premature; failure is decryption, padding invalid | Second independent reproduction: relay trace, diagnostic BCrypt build, LibTomCrypt cross-check | **Correct**, and later confirmed directly. |
| 9 | Driver passes `BCRYPT_BLOCK_PADDING` (`dwFlags = 0x1`) | `WINEDEBUG=+bcrypt` on a real launch, `steam-1867240.log:283` | **Measured.** Settles claim 4 as wrong and claim 8 as right. |
| 10 | Driver never reaches signature verification | Full driver-thread sequence in the same trace — no `BCryptImportKeyPair`, no `BCryptVerifySignature` | **Measured.** Eliminates claim 7 as the proximate cause: the code path never gets far enough to attest anything. |
| 11 | Wine's invalid-padding semantics diverge from Windows | Windows control run, 16/16 across 3 forms x 2 providers x 2 layouts | **Measured.** Fixed in patch 0005; verified 4/16 -> 16/16. |
| 12 | The padding-semantics fix will resolve WD-L014 | — | **Never claimed, and correctly predicted not to.** v2 run still produces WD-L014. |

---

## How it unfolded

**Sep 20, early — the kernel-API hypothesis.** The first `+ntoskrnl` trace disposed of
it: the driver loads and runs. The failure moved into the driver's own handler
(claims 1, 2).

**Sep 20, midday — decryption, briefly.** A `+bcrypt` trace showed the IOCTL
performing AES-CBC then ECDSA, and the note concluded "Wine produced wrong plaintext"
(claim 3). The same note recorded the call as using **no padding** (claim 4) — an
error that would cost the next two days.

**Sep 20, afternoon — the wrong turn.** Standalone tests were written to check the
crypto. They passed, so crypto was "ruled out" (claim 5), and with the buffer also
proven intact (claim 6), elimination pointed at environment attestation (claim 7).
That became the headline of a public write-up.

The tests could not have failed. They encrypted and decrypted through the same Wine
backend, never exercised the padded path, and returned 0 regardless. Claim 3 — which
was right — was discarded on their strength.

**Sep 20, evening — the second investigator.** An independent reproduction on
different hardware rejected claim 7 as beyond its evidence and produced a relay trace
showing `BCryptDecrypt` itself failing on invalid PKCS7 padding (claim 8). It also
identified precisely why the tests in claim 5 proved nothing. Claim 7 was retracted;
the public write-up was reworded to hold both readings open.

**Sep 20/21 — the Windows control.** A native Windows run with byte-identical
binaries succeeded, and measured a genuine Wine divergence on rejected padding
(claim 11). It could not settle the padding-flag question: user-mode tooling cannot
capture a kernel-mode BCrypt call.

**Sep 21 — the fix, and the measurement that mattered.** Patch 0005 brought Wine's
invalid-padding semantics in line with Windows, verified 4/16 -> 16/16, and shipped as
compat tool `wardogs-elytra-v2`. The game still failed with WD-L014 — as predicted
(claim 12), since Windows never reaches that branch.

The useful part was the trace taken alongside it. `BCryptDecrypt`'s existing TRACE
already prints `dwFlags` as its final argument — no instrumentation needed. One line
answered the question two documents had contradicted each other over (claim 9), and
the surrounding thread showed the driver ending at the decrypt with nothing after it
(claim 10).

---

| 13 | Linux fetched a different or corrupted module | — | **Eliminated.** All three cached CABs match the Windows-measured hashes on size and SHA-256. |
| 14 | Wine's crypto is wrong at the retail size | — | **Eliminated.** 55,275,056 B of openssl ciphertext decrypts in-place under Wine with zero mismatched bytes. |

## Where it stands

The proximate failure is **module decryption**, not environment attestation — measured,
not inferred. Two further candidates have since been eliminated by measurement
(claims 13 and 14), which narrows the defect to one stretch of pipeline:

```
CAB → [extract] → IOCTL buffer → [strip IV] → BCryptDecrypt
 ✅ verified                                    ✅ verified
          ↑_______ the defect is here _______↑
```

Given correct inputs Wine succeeds; the driver fails. So the ciphertext, key or IV is
wrong by the time it arrives. The plan for closing that is in the findings document
under *What remains, and the plan*, tracked as TASK-15 (observe the failure rather
than infer it), TASK-16 (compare the shipped ciphertext against what reaches the
driver) and TASK-17 (discriminate and fix).

This now looks like an ordinary Wine bug rather than a vendor wall — which is a
complete reversal of where the investigation started.

Attestation has **not** been disproven. It may well be a later gate. It simply is not
what produces WD-L014 today, and no claim about it should be made upstream.
