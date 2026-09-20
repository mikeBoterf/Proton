# Elytra-on-Proton diagnostic tests

Standalone tests that back the claims in `../wardogs-elytra-proton-findings.md`.
They isolate each thing Elytra's `elytraldrfs` driver does at the point it fails
(`WD-L014`), to prove the failure is **not** a Wine defect — every input Wine
provides is correct. These touch **nothing** belonging to Elytra; they exercise
Wine's own primitives with our own data.

Build with the repo's toolchain (`nix develop`, then MinGW), run under the built
Proton (`steam-run <compat-tool>/proton runinprefix <exe>`).

| File | Proves |
|---|---|
| `bcrypt_inplace_test.c` | Wine's **AES-128-CBC in-place decrypt** round-trips correctly at 48 B, 1 MB, and the full 52.7 MB (the exact op `elytraldrfs` performs). |
| `ecdsa_test.c` | Wine's **ECDSA P-384 / SHA-384 verify** accepts valid signatures and rejects corrupted ones. |
| `buftest.c` (driver) + `buftest_user.c` | A 52.7 MB `METHOD_BUFFERED` IOCTL is delivered to a driver **byte-perfect** (driver-side hash == user-side hash) — Wine's large-IOCTL marshaling is intact. |

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

Conclusion: crypto correct + buffer intact, yet the real driver still returns
`STATUS_UNSUCCESSFUL` — so the residue is `lighthouse`'s own direct kernel-authenticity
check, which honestly fails under Wine's userspace `ntoskrnl`. Vendor-gated, not fixable
client-side without faking the kernel (which this project does not do).
