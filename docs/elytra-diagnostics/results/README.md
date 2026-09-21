# Captured runs

`*-baseline.txt` — the `wardogs-elytra` compat tool as shipped (patches 0001-0004).
`*-patched.txt`  — same tool with a `bcrypt.dll` rebuilt from the submodule including
                   `nix/wine-patches/0005-bcrypt-Match-Windows-invalid-padding-semantics.patch`.

## padding-matrix

| | baseline | patched | Windows 26200.9457 |
|---|---|---|---|
| valid-small | 4/4 PASS | 4/4 PASS | PASS |
| invalid (3 forms x 2 providers x 2 layouts) | 12/12 FAIL | 12/12 PASS | reference |
| status on invalid padding | `0xc0000001` | `0xc000003e` | `0xc000003e` |
| `pcbResult` on invalid padding | 48 / 48 / 48 | 48 / 31 / 46 | 48 / 31 / 46 |
| IV advanced on rejection | no | yes | yes |

## bcrypt_inplace_test

Baseline showed `[AES-ECB 1MB] encrypt fail 0xc000000d`. That was a defect in the
test, not in Wine — it passed an IV for ECB, which both Wine and Windows reject with
STATUS_INVALID_PARAMETER. It had been failing silently for the whole investigation
because `main()` returned 0 unconditionally. Fixed by passing NULL/0 for ECB.

The valid openssl-generated fixture decrypts correctly under both builds
(`status=0 len=47`), matching Windows. This is the first evidence for Wine's padded
in-place decrypt that does not round-trip through Wine itself.

## What this does NOT show

None of this establishes a cause for WD-L014. On Windows the retail decrypt succeeds
and returns `0x00000000`, so Windows never reaches the branch whose status Wine was
reporting wrongly. See TASK-12.

## v2 compat tool

`padding-matrix-v2-tool.txt` is the same 16/16 result, but run through the installed
`wardogs-elytra-v2` compatibility tool rather than a scratch copy — i.e. the exact
path Steam invokes. Confirms the packaged tool carries the fix.

v1 (`wardogs-elytra`) is byte-for-byte unmodified; v2 is a full independent copy, not
hardlinked, so the two can be A/B'd without either affecting the other.

## bcrypt-driver-thread-trace.txt

The decisive measurement. Captured from a real launch on `wardogs-elytra-v2` with
`PROTON_LOG=1 WINEDEBUG=+bcrypt`, showing the elytraldrfs driver's complete BCrypt
sequence.

Two results:

1. `dwFlags = 0x1` (`BCRYPT_BLOCK_PADDING`) — settles the padding-flag question the
   two findings documents contradicted each other on. The independent review was
   right; the main document's "no padding" was wrong.
2. No `BCryptImportKeyPair`, no `BCryptVerifySignature` — the driver aborts at the
   decrypt and never reaches signature verification, and therefore never reaches
   anything that could attest the environment.

Pointer values and sizes only; this channel logs no key, IV, plaintext or ciphertext.

It is kept here deliberately. The postmortem faults the second investigation for
resting its claims on logs that were never published — this repository should not
repeat that.

## large-padded-test-v2.txt — the crypto is exonerated

The gap the rest of the suite left open: the 52.7 MB case in `bcrypt_inplace_test.c`
round-trips through Wine itself, and the openssl-backed fixtures were only 48 bytes.
Nothing tested Wine's padded in-place decrypt at the **retail size** against
**externally generated** ciphertext.

Now tested. openssl AES-128-CBC over 55,275,055 pattern bytes + one 0x01 PKCS7 byte,
decrypted in-place under `wardogs-elytra-v2` with `BCRYPT_BLOCK_PADDING`:

```
ObjectLength reported by Wine = 654
status    = 0
pcbResult = 55275055
plaintext mismatched bytes = 0 of 55275055
iv advanced to last ciphertext block = 1
RESULT: PASS
```

Exactly the retail shape — same size, same in-place layout, same padding flag —
and Wine gets it perfectly right.

Incidental confirmation: Wine reports `ObjectLength = 654`, which is exactly the
654-byte key object the retail driver allocates. The driver sized its buffer from
Wine's own answer, so there is no ObjectLength mismatch.

**Consequence.** Wine's AES-CBC padded in-place decrypt is correct at this exact
size and shape. If the driver's decrypt fails, the ciphertext, key or IV reaching
it must differ from what it should be. The crypto is not the bug; the data path
into it is.
