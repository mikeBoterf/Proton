# WARDOGS / Elytra on Proton — compatibility findings

**Status:** research / reproducer. **Not** a playable multiplayer fix, and by design
never will be one from the client side (see *Scope & ethics*).
**Target:** WARDOGS, Steam AppID `1867240` (Early Access), Windows build.
**Anti-cheat:** Elytra Anti-Cheat Framework (Vaiiya Corporate Limited / Embark).
**Base:** Valve Wine `proton_11.0` @ `dc26e6184` + a small patch series (below).

> **Read alongside:**
> [`wardogs-elytra-independent-review.md`](wardogs-elytra-independent-review.md)
> — a second investigation on different hardware, whose reading of the cause has
> since been **confirmed** by a `+bcrypt` trace (see *Two competing readings*).
> Where this document and that one disagreed, that one was right.
> [`wardogs-elytra-postmortem.md`](wardogs-elytra-postmortem.md) records how the
> wrong conclusion got as far as it did, and
> [`wardogs-elytra-timeline.md`](wardogs-elytra-timeline.md) is a dated ledger of
> every claim made and how each one resolved.

---

## TL;DR

With four Wine patches, a stock-built Proton runs Elytra's **real, unmodified**
launcher and driver stack considerably further than stock Proton: modules install,
`elytraldrfs_driver.sys` loads, creates its device, and receives its 52.7 MB IOCTL.
It then stops — the driver's own handler returns `STATUS_UNSUCCESSFUL`, surfacing as
**`WD-L014`**.

**No successful normal launch or multiplayer session has been achieved on this
path.** (A separate community *launcher-bypass* does reach a live match and gets
kicked at ~60 s on `no valid heartbeat within window` — but that path skips Elytra's
launcher entirely and does **not** exercise these patches. Earlier revisions of this
document blurred the two; see *How far it runs*.)

What the evidence supports:

- Every Windows kernel API Elytra's loader calls appears to be **serviced** by Wine —
  the driver loads and reaches its own logic.
- The 52.7 MB `METHOD_BUFFERED` IOCTL is **delivered intact** to the driver (verified
  with a purpose-built test driver: driver-side hash == user-side hash).
- Wine's padded in-place AES-CBC decrypt handles **externally generated** (openssl)
  ciphertext correctly at 48 bytes, and rejects malformed padding with the same
  status Windows uses (`docs/elytra-diagnostics/`, `results/`).

**Where it stops is now measured.** A `WINEDEBUG=+bcrypt` trace shows the driver's
entire crypto sequence ending at a single padded, in-place `BCryptDecrypt` of
55,275,056 bytes (`dwFlags = 0x1`). It then destroys the key and unloads — with **no
signature verification and nothing downstream**. The failure is at module decryption,
strictly before anything that could attest the environment.

That settles the disagreement between the two investigations in favour of the
companion review. An earlier revision of this document claimed the residue was a
direct kernel-authenticity check; **that claim is retracted** — the driver never runs
far enough for it to apply.

**Why the decrypt fails is still unknown**, and this document does not claim a root
cause. Environment attestation may still exist as a *later* gate; it simply is not
what produces `WD-L014` today. If it ever becomes the blocker, the only legitimate
unlock remains vendor-side (Embark enabling a Linux/Proton tier), as EAC/BattlEye did
with Valve.

---

## Scope & ethics (read this first)

This work is **compatibility engineering, not an anti-cheat bypass.**

- It runs the **real, unmodified** Elytra binaries. It does **not** patch Elytra,
  modify its downloaded modules, decrypt or dump its protected payloads, spoof or
  synthesize heartbeat/attestation data, stub checks to fake success, or disable any
  validation.
- The patches implement **genuine, missing Wine behavior** (kernel-driver hosting,
  a missing `ntdll` export, an `ntoskrnl` stub) so the real driver can *run* and
  return its *own* real result — which, under Wine, is an honest failure.
- We deliberately **did not** reverse-engineer Elytra's protected driver. The one
  step that would reveal more (dumping the decrypted `lighthouse` module from memory)
  is exactly the circumvention material we refuse to produce.

If Elytra's real driver runs and legitimately refuses under emulation, that is the
vendor's policy to change — not something to forge around.

---

## What this branch is

An **additive Nix layer** (flake-parts; see `nix/README.md`) plus a Wine patch series
in `nix/wine-patches/`. It does not replace Proton's build system — it uses Proton's
own `make redist` to produce a normal, redistributable Proton, and provides a
reproducible NixOS toolchain to do so. The result **runs**, gets Elytra's real driver
further than stock Proton, and documents precisely where and why it stops — so it can
serve as scaffolding if a Linux Elytra path is ever offered.

### The patch series (`nix/wine-patches/`)

Patches `0001`–`0003` are vendored from
[AstralDrift](https://github.com/AstralDrift/Proton-WARDOGS-Elytra) (same base commit,
apply cleanly); `0004` is ours. All make the real driver *run*; none touch Elytra.

| # | What it does | Why |
|---|---|---|
| 0001 | Real `NtLoadDriver` → SCM → `winedevice`; `RtlStringFromGUIDEx`; `ksecdd` BCrypt forwarding; IRP security context / create disposition | `NtLoadDriver` was a stub; `RtlStringFromGUIDEx` was missing (launcher `WD-L017` abort); the driver needs BCrypt in kernel |
| 0002 | Padded in-place symmetric decrypt fix (+ regression test) | The driver decrypts its module in-place via BCrypt |
| 0003 | `MmMapLockedPagesSpecifyCache` | Was a stub the driver needs |
| 0004 | Guard `DEFAULT_SECURITY_COOKIE_64` under `_WIN64` | 0001 defined it unconditionally → 32-bit `ntoskrnl` tripped `-Werror=type-limits` in Proton's build |

Plus one runtime workaround, **not** a patch: a `dosdevices/volume{…0043}` symlink so
Wine resolves the `\??\Volume{GUID}\…` paths Elytra opens (`0x43` = drive `C:`). The
proper fix is a real Wine `\??\Volume{GUID}` resolver (see *Remaining work*).

---

## Elytra architecture (as observed, from cleartext metadata only)

- **Service:** `Elytra.Service` (`C:\Program Files\Elytra\service.exe`, Rust /
  jsonrpsee over a named pipe). CLI front-end `control.exe`
  (`status | certificates | modules {add,check,is-installed} | session launch`).
- **Distribution:** modules fetched from `https://elytra.ac/v1/env/<id>/cfg?s=<hash>`
  (manifest) and `https://elytra.ac/v1/data/<blob>.cab?s=<hash>`. Signed by
  `Vaiiya Corporate Limited` (thumbprint `AB9527FA0A67115A28799C0D7551BBE42190A0EE`).
- **Modules** (cleartext `module.json` inside each cab):

  | id | role | ships |
  |---|---|---|
  | `23da072e-f99b-4807-90e0-5670036c3261` (rev 10) | `elytraldrfs` — loader-filesystem | `elytraldrfs_shared.dll` + `elytraldrfs_driver.sys` |
  | `0b9c0968-c6e4-4ce1-87d7-28e0455473ea` (rev 3) | `heartbeat` — user-mode | `heartbeat.dll` (no driver) |
  | `597ca429-3abf-43b2-aa78-73c8d7524be7` (rev 45) | `lighthouse` — main scanner | `lighthouse_driver.sys` (`encrypted=1 compressed=1`, `singleton=EmbarkLighthouse`) + `lighthouse_module.dll` |

- **Launcher step machine:** `init → fetch-config → download-modules → connect-service
  → install-modules → session-create → session-configure → session-prime →
  start-notify → launch`. Error codes `WD-L000…WD-L022`.

---

## How far it runs, and where it stops

With the launcher run normally (no bypass) on the patched Proton:

1. ✅ Launcher runs (didn't crash — `RtlStringFromGUIDEx` present).
2. ✅ Modules download, verify (signature OK), install — after the `C:` volume-GUID
   symlink; without it, `install-modules` fails as **`WD-L010`**.
3. ✅ Service connects; session create + configure.
4. ✅ `NtLoadDriver` → `winedevice`; `elytraldrfs_driver.sys` + `ksecdd.sys` load;
   `DriverEntry` runs; `IoCreateDevice \Device\elytra_<rand>` succeeds; module opens it.
5. ⛔ `elytraldrfs` sends itself one IOCTL and its **own handler returns
   `STATUS_UNSUCCESSFUL`** → `IElytraModule::load` fails → session-prime fails →
   **`WD-L014`**.

(For reference: running with the community *launcher-bypass* — `exec`-swapping the
launcher for the game client — the game runs and is kicked at exactly 1:00 with
`no valid heartbeat within window`, because the bypass skips Elytra's launcher
entirely. That path does **not** exercise these patches. The `WD-L014` above is the
patched launcher genuinely running Elytra.)

---

## Where it stops — what the driver exercises

What follows is what the driver is observed to *do* before it fails. The kernel-API
surface below is serviced by Wine well enough for the driver to load and reach its
own logic; that much is directly observable in the traces. What it does **not**
establish on its own is *why* the driver then refuses — see *Two competing readings*.

### The IOCTL that fails
`elytraldrfs` device-control code **`0x222014`** = `FILE_DEVICE_UNKNOWN` (0x22),
function `0x805`, `METHOD_BUFFERED`, `FILE_ANY_ACCESS`. `in_size = 55,275,072`
(the encrypted+compressed `lighthouse` module), `out_size = 64` (result on success).

### Kernel APIs `elytraldrfs` invokes — all serviced by Wine's `ntoskrnl` handlers
From a `+ntoskrnl` trace (representative counts):
`IoCreateDevice`, `IoCreateDriver`, `IoCreateSymbolicLink`, `IoAttachDeviceToDeviceStack`,
`IoBuildSynchronousFsdRequest` / `IoBuildAsynchronousFsdRequest`,
`IoBuildDeviceIoControlRequest`, `IoAllocateMdl` / `IoFreeMdl`, `ZwLoadDriver`,
`ObReferenceObjectByName` / `ObReferenceObject`, `KeInitializeEvent` / `KeSetEvent`,
spinlock + critical-region primitives, `ExAllocatePool2`, IRP alloc/init/complete/free.
These are standard loader/device/IRP calls; **Wine handles them all**, which is why
the driver loads and reaches its own logic.

### The crypto it performs inside the IOCTL — and what our tests do and don't show
The driver decrypts the `lighthouse` module and verifies signatures:
- `BCryptOpenAlgorithmProvider("AES")` → `BCryptSetProperty("ChainingMode", "ChainingModeCBC")`
  → `BCryptGenerateSymmetricKey` (16-byte key) → **`BCryptDecrypt` in-place** (input ptr
  == output ptr, 55,275,056 bytes, 16-byte IV, `BCRYPT_BLOCK_PADDING`).
- `BCryptImportKeyPair("ECCPUBLICBLOB", 104 B = P-384)` → `BCryptVerifySignature`
  (SHA-384, 96-byte signature), repeatedly.

> **RESOLVED — the driver passes `BCRYPT_BLOCK_PADDING`.** Measured directly from a
> `WINEDEBUG=+bcrypt` run against the patched build:
>
> ```
> BCryptDecrypt 0xC221E0, 0xD20050, 55275056, (null), 0xC1F820, 16,
>               0xD20050, 55275056, 0xC1F75C, 0x1
> ```
>
> The final field is `dwFlags` = `0x1` = `BCRYPT_BLOCK_PADDING`. The same line
> confirms the call is in-place (input pointer == output pointer == `0xD20050`) with
> a 16-byte IV over 55,275,056 bytes (3,454,691 whole blocks).
>
> An earlier revision of this document recorded "no padding". **That was wrong.** The
> companion review was right.

Standalone tests (sources in `docs/elytra-diagnostics/`) under this exact Wine:
- AES-CBC in-place decrypt round-trips at 48 B, 1 MB, and the full 52.7 MB.
- ECDSA P-384 / SHA-384 verify passes valid signatures and rejects corrupted ones.

**Limits of those tests** (fairly raised by the companion review — verify against
`bcrypt_inplace_test.c` yourself):
- They encrypt *and* decrypt through the same Wine backend, so they demonstrate
  **self-consistency, not equivalence to Windows**. An implementation wrong in a
  symmetric way would still pass. Calling this "bit-for-bit correct" — as earlier
  revisions did — overstated it.
- They pass `dwFlags = 0`, so they **do not exercise the padded path** at all. If the
  retail call is padded, these tests miss the failing branch entirely.
- `main()` returns `0` unconditionally and discards each `test_mode` result, so the
  exit code carries no signal; read the printed PASS/FAIL lines.

Closing those gaps — externally generated fixtures, an exact-size padded in-place
case, and a negative padding test — is the most valuable next step on the Linux side.

### The buffer delivery — delivered intact
A minimal test kernel driver that FNV-hashes a large `METHOD_BUFFERED` IOCTL input
received the full **55,275,056 bytes intact** (driver-side hash == user-side hash).
The hash is computed independently on each side of the boundary, so this is the
best-supported of the three tests: Wine's large-IOCTL marshaling is not truncating or
corrupting the buffer.

## Two competing readings — now decided

Both investigations saw the same symptom — `STATUS_UNSUCCESSFUL` out of the driver's
own handler, surfacing as `WD-L014` — and disagreed on the cause. A `+bcrypt` trace
of the full driver thread settles it.

> **Reading B is the proximate cause. Reading A is eliminated as the immediate
> explanation.** The complete BCrypt sequence on the driver's thread is:
>
> ```
> BCryptOpenAlgorithmProvider  L"AES"
> BCryptSetProperty            L"ChainingMode"   (32 bytes)
> BCryptGetProperty            L"ObjectLength"
> BCryptGenerateSymmetricKey   secret 16 B, object buffer 654 B
>                              fixme: ignoring object buffer
> BCryptDecrypt                55,275,056 B, in-place, flags 0x1
> BCryptDestroyKey
> BCryptCloseAlgorithmProvider
> ```
>
> There is **no `BCryptImportKeyPair` and no `BCryptVerifySignature`**. The driver
> tears down immediately after the decrypt and `NtUnloadDriver` follows. It never
> reaches signature verification, so it never reaches anything that could attest the
> kernel. Whatever `lighthouse` may check, the failure happens strictly before it.
>
> Still unknown: **why** the decrypt fails. `BCryptDecrypt` traces entry only, so the
> return status is not in that log; the absence of downstream verification is strong
> structural evidence, not a logged status.

The two readings as originally stated:

### A — Environment attestation (the reading originally recorded here)
*Argued by elimination.* If every input Wine provides is correct, what remains is
`lighthouse`'s **own environment verification**: kernel anti-cheats attest by reading
kernel memory and structures directly (module lists, kernel image, page state,
CPU/hypervisor signals) — *not* via API calls we can service. Under Wine there is no
real kernel; `ntoskrnl` is a userspace reimplementation hosted in `winedevice.exe`,
with no ring-0, no genuine kernel structures, no hardware root of trust. The check
would then honestly evaluate to "this is not a genuine NT kernel" and the driver
would refuse.

If A is right: *it's running — it just doesn't know what to make of a kernel that
isn't a true-blue Windows NT kernel.* Not a spoof, not a Wine defect. **The only way
past it would be to fake the kernel itself, which is the forgery line we do not
cross**, making this vendor-side by definition.

**Outcome:** eliminated as the proximate cause. The driver never executes far enough
to attest anything. A remains possible as a *further* gate that would be met if the
decrypt were fixed — it has not been disproven, only shown not to be what produces
WD-L014 today.

### B — Module decryption fails first (the companion review's reading)
*Argued from an observed return value.* A filtered relay trace shows `BCryptDecrypt`
itself returning `0xc0000001` before the driver propagates the same status, and a
diagnostic BCrypt build points at **invalid PKCS7 padding** as the failing branch. A
second AES implementation (LibTomCrypt) reportedly agrees with the GnuTLS backend on
the failing final block, which would push the problem toward the *inputs* or upstream
context rather than backend arithmetic.

If B is right, the failure sits **upstream of any environment check** — the module
never decrypted, so `lighthouse` never ran to attest anything, and A's conclusion is
premature.

**Outcome:** supported. The padded in-place call is confirmed, and the driver aborts
at exactly that point. **Still not a root cause:** it identifies where the failure
surfaces, not why the padding is invalid.

### What remains, and the plan

The `dwFlags` question is answered and reading A is eliminated as the proximate
cause. Two further eliminations followed, both by direct measurement:

- **The module data is identical to Windows.** All three cached Elytra CABs match the
  Windows-measured hashes on size and SHA-256, including the 55,329,835-byte
  `lighthouse` CAB. Linux did not fetch something different or corrupt.
- **Wine's crypto is correct at the retail size and shape.** 55,275,056 bytes of
  openssl-generated ciphertext, decrypted in-place with `BCRYPT_BLOCK_PADDING`:
  status `0x0`, `pcbResult` 55,275,055, zero mismatched plaintext bytes. Exactly the
  operation the driver performs. (Wine also reports `ObjectLength = 654` — precisely
  the key-object size the driver allocates, so there is no mismatch there either.)

That leaves a narrow box:

```
CAB 55,329,835 → [extract/decompress] → IOCTL 55,275,072 → [strip 16B IV] → BCrypt 55,275,056
     ✅ identical to Windows                                                  ✅ cipher correct
                        ↑________________ the defect is in here ________________↑
```

Given correct inputs Wine succeeds, and the driver fails — so what reaches
`BCryptDecrypt` is not what should reach it. **The ciphertext, the key, or the IV is
wrong by the time it arrives.**

#### Plan

1. **Observe the failure instead of inferring it** ([TASK-15](../backlog/tasks)).
   `+bcrypt` traces entry only; the failure is still inferred from the driver aborting.
   Add a targeted trace to the padding-validation branch of our own build logging the
   observed padding length and status — nothing else — and ship it as `v3`. A wild
   padding length means garbage plaintext; a plausible one means something subtler;
   success would mean the current model is wrong and must be re-derived.
2. **Localise the corruption** ([TASK-16](../backlog/tasks)). Extract
   `lighthouse_driver.sys` from the CAB natively on Linux, outside Wine, and compare
   its digest against the buffer that actually reaches the driver. The CAB is a plain
   container and its payload stays encrypted throughout — this is an integrity check
   on a copy, not an inspection of protected content.
3. **Discriminate and fix** ([TASK-17](../backlog/tasks)). If the ciphertext differs,
   the defect is in extraction, file I/O, the usermode→IOCTL copy, or
   `MmMapLockedPagesSpecifyCache`; the first-divergence offset discriminates. If it
   matches, the key or IV is mis-derived — and
   `BCryptGenerateSymmetricKey ignoring object buffer` sits on exactly this sequence,
   flagged three times and never examined.

Throughout: digests and lengths of **ciphertext** only. The decrypted module is never
logged or inspected. We are checking that bytes survive a copy, not learning what they
mean.

This document still does **not** claim a root cause. But the failure is now localised
to a short, inspectable stretch of Wine, and looks like an ordinary bug rather than a
wall.

---

## How this would work if Embark offered a Linux path

*This section assumes reading A, which the trace has shown is **not** the current
blocker — the driver fails at decryption and never reaches attestation. It is kept
because it remains the right analysis **if** the decrypt is ever fixed and an
attestation gate turns out to sit behind it. Treat it as contingency, not as a
description of today's failure.*

Granting A: a Windows kernel driver fundamentally cannot attest under Wine (no real
kernel). A Linux path would then **not** be "make `lighthouse.sys` pass" — it would be
a different, vendor-owned enforcement tier. Three shapes, in increasing order of
realism for Embark:

1. **Native Linux runtime + Proton bridge (the EAC/BattlEye model).** Proton already
   ships `eac-bridge` and `battleye-bridge` — thin shims that forward a game's
   anti-cheat calls to the vendor's **own signed native-Linux runtime**. There is **no
   `elytra-bridge`** in Proton today. If Embark shipped a Linux Elytra runtime (doing
   Linux-appropriate checks via `/proc`, kernel interfaces, etc.) and coordinated a
   bridge with Valve, the game would talk to *that* instead of `lighthouse.sys`.

2. **Usermode tier + server-side behavioral detection (most likely).** Embark has
   publicly pivoted toward **server-side behavioral analysis** and stated Proton
   support will remain (with stricter requirements). The lowest-effort unlock needs
   **zero Proton/Wine changes**: run only Elytra's *usermode* components on Proton and
   have the **server accept that tier** for the title, relying on behavior detection
   instead of the kernel driver. Notably, on WARDOGS under this branch the entire
   usermode chain already works — only the kernel-driver heartbeat is demanded.

3. **Hardware-rooted client attestation (sound but restrictive).** Verify the Linux
   client is running known-good software via **TPM-measured boot + remote attestation**
   (NixOS supports this: lanzaboote + TPM2 measured boot; reproducible nixpkgs builds
   make the known-good set enumerable and even byte-verifiable). This is the rigorous
   form of "prove the kernel is authentic," but it requires a hardware root of trust
   *and* whitelisting kernels/modules — which excludes most Linux players (custom
   kernels), so it scales poorly. This is precisely why Embark chose behavioral (2).

### Where this branch fits in
It is a **faithful reproducer** and a possible **foundation**. It shows Proton can
carry Elytra's real launcher / module / service / driver-load surface up to the IOCTL,
and documents exactly what the driver exercises along the way. It now also localises
the failure precisely: one padded in-place `BCryptDecrypt`, with nothing downstream of
it executed. What it does **not** establish is why that decrypt fails. If Embark ever
ships a Linux runtime (1) or enables a usermode/behavioral tier (2), the client-side
plumbing this branch fixes is already in place. It **cannot** and is **not intended
to** make the current kernel-driver tier pass.

---

## Reproducing

See `nix/README.md` for the full NixOS build recipe (the notable NixOS gotchas:
run under `steam-run` for `/bin/bash`; `git submodule update --init --recursive`;
pre-fetch the gecko/mono/xalia blobs; `--docker-opts "--dns 1.1.1.1"` so the build
container can resolve hostnames). Output installs as a normal Steam compatibility tool.

The standalone AES / ECDSA / buffer-marshaling test sources are in
`docs/elytra-diagnostics/` (with build/run instructions) and demonstrate each claim
above. Full `PROTON_LOG` / `+ntoskrnl` / `+bcrypt` traces were captured during the
investigation; they are large and kept out of the repo.

---

## Remaining, in-bounds work

- **Settle the padding-flag question** (above) from existing relay traces, then
  reconcile the two documents into a single account.
- **Obtain a matched Windows baseline** — same launcher and BuildID, capturing the
  BCrypt call parameters and the return path. This is the blocking artifact.
- **Close the crypto test gaps:** externally generated fixtures, an exact-size padded
  in-place decrypt, a negative padding case, and a meaningful exit code.
- **Report to the vendor** — *once a cause is actually established.* The constructive
  ask to Embark / Valve
  ([ValveSoftware/Proton#10113](https://github.com/ValveSoftware/Proton/issues/10113),
  [#10163](https://github.com/ValveSoftware/Proton/issues/10163)) is for a
  Proton/usermode tier for WARDOGS as for The Finals — but reporting a *specific*
  root cause before it is nailed down would be worse than reporting nothing.
- **Upstreamable Wine fix:** a real `\??\Volume{GUID}` resolver (resolve the GUID
  against each drive's volume serial) to replace the `dosdevices` symlink workaround —
  useful to *every* Windows app that uses volume-GUID paths, independent of Elytra.

---

## A note on the bigger picture

Client-side anti-cheat trust — even Windows' hardware-signed kernel model — is a
cost/deterrence gradient, not a boundary (see BYOVD, stolen certs, hypervisor cheats).
Wine doesn't defeat it; it just honestly declines to impersonate a kernel it isn't.
The durable answer, on every platform, is **assume the client is hostile and detect
cheating from server-observable behavior** — which is the direction Embark has stated
it is taking. That, not a client-side patch, is what will eventually let Linux players
in — honestly.
