# Elytra-on-Proton diagnostic tests

Standalone tests that accompany `../wardogs-elytra-proton-findings.md`. They isolate
individual things Elytra's `elytraldrfs` driver does at the point it fails
(`WD-L014`). They touch **nothing** belonging to Elytra; they exercise Wine's own
primitives with our own data.

> **Read the limits before citing these.** The crypto tests below are weaker evidence
> than earlier revisions of the findings document claimed, and the companion review
> ([`../wardogs-elytra-independent-review.md`](../wardogs-elytra-independent-review.md))
> was right to say so. See *What these do and don't establish*.

Build with the repo's toolchain (`nix develop`, then MinGW), run under the built
Proton (`steam-run <compat-tool>/proton runinprefix <exe>`).

| File | Exercises |
|---|---|
| `bcrypt_inplace_test.c` | Wine's AES-128-CBC in-place decrypt, round-tripped at 48 B, 1 MB, and the full 52.7 MB. |
| `ecdsa_test.c` | Wine's ECDSA P-384 / SHA-384 verify — accepts valid signatures, rejects corrupted ones. |
| `buftest.c` (driver) + `buftest_user.c` | A 52.7 MB `METHOD_BUFFERED` IOCTL delivered to a driver, driver-side hash vs user-side hash — tests Wine's large-IOCTL marshaling. |

## What these do and don't establish

**`buftest` is the strongest of the three.** It computes the hash independently on
each side of the boundary, so a match is real evidence that Wine's large-IOCTL
marshaling delivers the buffer intact.

**The crypto tests are weaker than they look:**

- `bcrypt_inplace_test.c` encrypts *and* decrypts through the same Wine backend
  (lines 43 and 51). That shows **self-consistency, not equivalence to Windows** — an
  implementation wrong in a symmetric way passes cleanly. It is not sufficient to call
  Wine's AES "bit-for-bit correct".
- Both `BCryptEncrypt` and `BCryptDecrypt` are called with `dwFlags = 0`, so **the
  padded path is never exercised.** The companion review reports the retail driver
  passing `BCRYPT_BLOCK_PADDING` and failing on invalid PKCS7 padding — if so, this
  test misses the failing branch entirely. Which flag the retail call uses is
  currently unresolved.
- `main()` returns `0` unconditionally and discards every `test_mode` return value
  (line 72), so **the exit code carries no signal.** Read the printed PASS/FAIL lines;
  don't script on `$?`.

Wanted: externally generated fixtures (not round-tripped through Wine), an exact-size
padded in-place decrypt, a negative padding case, and a real exit code.

Build examples:

```sh
# user-mode tests
x86_64-w64-mingw32-gcc bcrypt_inplace_test.c -o bcrypt_t.exe -lbcrypt
x86_64-w64-mingw32-gcc ecdsa_test.c          -o ecdsa_t.exe  -lbcrypt

# test driver (needs the ddk include dir on the path)
DDK=$(dirname $(find /nix/store/*mingw32-gcc*/x86_64-w64-mingw32/sys-include/ddk/wdm.h | head -1))
x86_64-w64-mingw32-gcc -O2 -nostdlib -shared -o buftest.sys buftest.c -I"$DDK" \
  -Wl,--subsystem,native -Wl,--entry,DriverEntry -Wl,--image-base,0x140000000 -lntoskrnl
x86_64-w64-mingw32-gcc -O2 buftest_user.c -o buftest_user.exe
# load: sc create BufTest type= kernel binPath= C:\...\buftest.sys && sc start BufTest
```

## Status

The buffer arrives intact and the crypto primitives are self-consistent, yet the real
driver still returns `STATUS_UNSUCCESSFUL`. **Why is unresolved.** The two candidate
explanations — `lighthouse` attesting the running kernel (unsatisfiable under Wine's
userspace `ntoskrnl`, therefore vendor-gated) versus module decryption failing first
on invalid padding — are laid out in *Two competing readings* in the findings
document. These tests do not currently distinguish between them.

Whatever the cause, this project does not fake the kernel or forge attestation.
