# WARDOGS / Elytra — independent review and second reproduction

**Companion to [`wardogs-elytra-proton-findings.md`](wardogs-elytra-proton-findings.md)
— read both.** Dated September 20, 2026.

This is a **second, independently conducted investigation** on different hardware,
plus a review of the main findings document. It reproduces the same progress under
patched Proton but reaches a **different conclusion about the cause**, and it
identifies gaps in the crypto tests the main document relies on. Those critiques have
been accepted and the main document has been revised accordingly.

**This document's reading has since been CONFIRMED.** A `WINEDEBUG=+bcrypt` trace of a
real launch shows the retail driver calling `BCryptDecrypt` with `dwFlags = 0x1`
(`BCRYPT_BLOCK_PADDING`) — exactly as claimed here, and contrary to the "no padding"
recorded in the main findings document. The same trace shows the driver performing no
signature verification at all after the decrypt, which eliminates the environment-
attestation reading as the proximate cause of WD-L014. Details in *Two competing
readings* in the main document and in backlog TASK-12.

What remains unknown is **why** the decrypt fails. That is not answered by either
document. See [`wardogs-elytra-timeline.md`](wardogs-elytra-timeline.md) for the full
claim ledger and [`wardogs-elytra-postmortem.md`](wardogs-elytra-postmortem.md) for
how the original conclusion survived as long as it did.

The positive claims below (the relay trace, the diagnostic BCrypt build, the
LibTomCrypt cross-check) rest on logs held locally and not published here; they should
be held to the same standard this document rightly applies to the main one.

---

**Not playable yet.** The normal, unmodified launcher progresses past module
installation and loads the real Elytra loader driver under patched Proton. Its
module-decryption operation fails with invalid PKCS7 padding, producing WD-L014.
No successful game launch or multiplayer session was achieved.

## Verified locally

Game: Steam AppID 1867240, BuildID 25278619, launcher 1.2.
Machine: AMD Navi 22, amdgpu/RADV, Linux 7.2.5-3-omarchy.

1. Stock Proton Experimental `experimental-11.0-20260910b-x86_64` fails
   `InstallModule` with file-not-found through a Volume GUID path (WD-L010).
   `logs/01-stock-WD-L010.log` contains the actual service RPC failure.
2. A compiled Windows probe reproduces the same failed file open. Adding a
   `dosdevices/volume{00000000-0000-0000-0000-000000000043} -> c:` alias makes
   that probe pass. The game's C:, S:, and Z: aliases are recorded in
   `volume-aliases-added.json`. All three real Elytra modules then install.
3. Stock Proton next encounters missing `RtlStringFromGUIDEx` and a stub
   `NtLoadDriver`. See `logs/02-volume-fixed.log`.
4. The published AstralDrift compatibility build supplies the first three patches
   vendored by mikeBoterf's branch. Its download matched its published SHA-256.
   It successfully starts the driver, creates its device, and dispatches IOCTL
   `0x222014` with 55,275,072 input bytes. See
   `logs/03-patched-proton-WD-L014.log`.
5. A filtered return-value trace shows `BCryptDecrypt` returning `0xc0000001`
   before the driver propagates the same status. This is not merely a generic
   driver failure. See `logs/05-relay-decrypt-failure.log`.
6. A diagnostic BCrypt DLL built from the exact patched Wine source identifies
   **invalid PKCS7 padding length** as the failing branch.
7. A second AES implementation (LibTomCrypt, already bundled in Wine) independently
   calculates the final CBC block using the same key and ciphertext. It **matches**
   the GnuTLS backend on the actual game operation. Padding remains invalid.
   See `logs/07-independent-backend-confirmation.log`, especially the
   `decrypt diagnostic:` lines. No key, IV, ciphertext, or plaintext was logged.

The final observed launcher error was `WD-L014-7538de5103e6`.

## Scope of the conclusion

The tested decryption backend agrees with an independent implementation on the
failing final block. That narrows the problem to the supplied cryptographic
inputs or upstream execution/context, rather than a demonstrated AES backend
arithmetic defect. It does **not** establish why those inputs produce invalid
padding, prove every Wine API correct, or prove a specific kernel-authenticity
check has executed. A matched Windows trace or vendor information is unavailable.
Skipping padding validation would not establish correct decryption.

## Review of the main findings document

Reviewed `mikeBoterf/Proton`, branch `wardogs-elytra-nix`, commit
`a295d0bdce984b7d9656cc04767902959395ad4e`. The points below refer to that commit;
the companion document has since been revised to reflect them:

- Its Wine patches implement genuine missing behavior: driver hosting through
  SCM/winedevice, GUID string conversion, BCrypt forwarding and overlap handling,
  driver security cookies, create-IRP fields, and locked-page mapping. These
  provided useful, observable progress here.
- Its findings document is internally inconsistent: the opening describes
  reaching a server heartbeat, but the detailed normal-launch sequence stops at
  WD-L014; the approximately one-minute match described later uses a separate
  launcher-bypass path. It is not evidence that the patched normal launch works.
- Its assertion that the remaining error must be a direct kernel-authenticity
  check goes beyond the supplied evidence. Our real return trace instead
  identifies a decryption failure before that conclusion can be drawn.
- Its large AES test uses **no padding**, whereas this retail driver passes
  `BCRYPT_BLOCK_PADDING`. It also round-trips through the same backend and exits
  zero regardless of its individual test results. Those tests do not establish
  Windows-equivalent behavior for this actual call.
- We closed that test gap with independently encrypted fixtures, exact-size
  padded in-place decryption, and a negative padding test. All passed.
- The branch's fourth patch is a 32-bit compile guard, not a new runtime fix for
  the 64-bit failure observed here. We tested the original author's published
  build of the shared first three patches, not a fresh full build of mikeBoterf's
  Nix configuration.

Sources:
- https://github.com/mikeBoterf/Proton/tree/wardogs-elytra-nix
- https://github.com/mikeBoterf/Proton/blob/a295d0bdce984b7d9656cc04767902959395ad4e/docs/wardogs-elytra-proton-findings.md
- https://github.com/AstralDrift/Proton-WARDOGS-Elytra/releases/tag/v0.1.0-playtest
- https://github.com/AstralDrift/wine/tree/wardogs-elytra-proton-compat
- https://github.com/ValveSoftware/Proton/issues/10113

## Reproducers and retained files

- `probe.c` / `probe.exe`: Windows volume-path and API-export test.
- `crypto-probe.c` / `crypto-probe.exe`: independent AES fixtures, 48-byte,
  1-MiB and 55,275,056-byte ciphertexts; valid data and invalid padding.
- `generate-fixtures.py`: creates fixture data using Python cryptography/OpenSSL.
- `run-probe.sh` / `run-crypto-probe.sh`: isolated prefixes; do not run concurrently
  because both use `logs/steam-0.log`. Saved named logs preserve the results.
- `review/bcrypt-diagnostics.patch`: diagnostic-only Wine changes, preserving
  validation and return values. `source/` and `build-wine/` retain source/build.
- `backups/`: original Steam configs, pre-custom-runtime game prefix, and original
  BCrypt DLLs. Config backups contain private Steam data; do not publish them.

Probe compilation used Clang's `x86_64-w64-windows-gnu` target, Wine headers and
import libraries, `-fuse-ld=lld -fno-stack-protector -nostdlib`, and entry point
`mainCRTStartup`. The crypto probe links `kernel32`, `bcrypt`, and `ucrtbase`.
The diagnostic Wine build used `make -j10 dlls/bcrypt/x86_64-windows/bcrypt.dll`.

## Installed state after investigation

WARDOGS remains installed, with its volume aliases and the published custom
`proton-wardogs-elytra` compatibility tool selected for this game only. The library
root is explicit in its launch options so Proton recreates the required S: drive.
Temporary diagnostic DLLs and relay filtering are restored/removed on completion;
the diagnostic source, build, patch, and logs remain here for review.
Other games' compatibility selections were not changed.
