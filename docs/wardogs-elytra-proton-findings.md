# WARDOGS / Elytra on Proton — compatibility findings

**Status:** research / reproducer. **Not** a playable multiplayer fix, and by design
never will be one from the client side (see *Scope & ethics*).
**Target:** WARDOGS, Steam AppID `1867240` (Early Access), Windows build.
**Anti-cheat:** Elytra Anti-Cheat Framework (Vaiiya Corporate Limited / Embark).
**Base:** Valve Wine `proton_11.0` @ `dc26e6184` + a small patch series (below).

---

## TL;DR

With four Wine patches, a stock-built Proton runs Elytra's **real, unmodified**
launcher and driver stack on Linux **all the way to the server heartbeat** — the
game launches, joins a live match, and plays for ~60 s before the server kicks with
`no valid heartbeat within window`. We then isolated *exactly* why, by elimination:

- Every Windows kernel API Elytra's loader calls is **serviced correctly** by Wine.
- The crypto it uses (AES-128-CBC in-place decrypt, ECDSA P-384 / SHA-384 verify) is
  **bit-for-bit correct** under Wine (verified with standalone tests).
- The 52.7 MB `METHOD_BUFFERED` IOCTL is **delivered byte-perfect** to the driver
  (verified with a purpose-built test driver).

Yet the driver returns `STATUS_UNSUCCESSFUL`. The residue is **not a Wine bug and not
anything the client can honestly change**: after decrypting its module correctly,
Elytra's `lighthouse` component inspects the *running kernel* directly to attest a
genuine, unmodified ring-0 NT kernel — and Wine has no real kernel (its `ntoskrnl`
is a userspace reimplementation), so the check honestly fails. There is nothing wrong
to fix; the environment simply isn't a Windows kernel, and Elytra correctly says so.

**The only legitimate unlock is vendor-side** (Embark enabling a Linux/Proton
enforcement tier for the title), exactly as EAC/BattlEye did with Valve. This branch
is a faithful reproducer of the gap and a possible foundation to *build on* if that
path is ever offered.

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

## The exact failure — which items call the Wine kernel handlers

This is the heart of it: **everything Elytra asks the Windows kernel is handled
correctly by Wine.** The failure is not in what Wine returns; it is that the driver,
having received correct answers, then verifies the *nature of the kernel itself* and
correctly finds it is not a genuine ring-0 NT kernel.

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

### The crypto it performs inside the IOCTL — proven correct under Wine
The driver decrypts the `lighthouse` module and verifies signatures:
- `BCryptOpenAlgorithmProvider("AES")` → `BCryptSetProperty("ChainingMode", "ChainingModeCBC")`
  → `BCryptGenerateSymmetricKey` (16-byte key) → **`BCryptDecrypt` in-place** (input ptr
  == output ptr, 55,275,056 bytes, 16-byte IV, no padding).
- `BCryptImportKeyPair("ECCPUBLICBLOB", 104 B = P-384)` → `BCryptVerifySignature`
  (SHA-384, 96-byte signature), repeatedly.

Standalone tests (sources in `docs/elytra-diagnostics/`) under this exact Wine:
- **AES-CBC in-place decrypt round-trips correctly** at 48 B, 1 MB, and the full 52.7 MB.
- **ECDSA P-384 / SHA-384 verify** passes valid signatures and rejects corrupted ones.

### The buffer delivery — proven byte-perfect
A minimal test kernel driver that FNV-hashes a large `METHOD_BUFFERED` IOCTL input
received the full **55,275,056 bytes intact** (driver-side hash == user-side hash). So
Wine's large-IOCTL marshaling is not truncating or corrupting anything.

### Therefore: the residue is a direct kernel-authenticity check
Every input Wine provides is correct; the driver still returns `STATUS_UNSUCCESSFUL`.
By elimination, the failure is `lighthouse`'s **own environment verification**: a
kernel anti-cheat attests authenticity by **reading kernel memory/structures
directly** (module lists, kernel image, page state, CPU/hypervisor signals) — *not* via
API calls we can service. Under Wine there is **no real kernel**: `ntoskrnl` is a
userspace reimplementation hosted in `winedevice.exe`, with no ring-0, no genuine
kernel structures, no hardware root of trust. So the check honestly evaluates to
"this is not a genuine NT kernel," and the driver refuses.

Put plainly, in the requester's words: *it's running — it just doesn't know what to
make of a kernel that isn't a true-blue Windows NT kernel.* Not a spoof, not a Wine
defect — an environment the driver was never built to accept, returning an honest
failure. **The only way past it is to fake the kernel itself, which is the forgery
line we do not cross.**

---

## How this would work if Embark offered a Linux path

A Windows kernel driver fundamentally cannot attest under Wine (no real kernel). So a
Linux path is **not** "make `lighthouse.sys` pass" — it is a different, vendor-owned
enforcement tier. Three shapes, in increasing order of realism for Embark:

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
It is a **faithful reproducer** and a **foundation**. It proves Proton can carry
Elytra's real launcher/module/service/driver-load surface end-to-end and pinpoints the
single remaining gap (the kernel-authenticity check). If Embark ever ships a Linux
runtime (1) or enables a usermode/behavioral tier (2), the client-side plumbing this
branch fixes is already in place, and the traces here document exactly what the driver
exercises. It **cannot** and is **not intended to** make the current kernel-driver tier
pass.

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

- **Report to the vendor.** The constructive ask to Embark / Valve
  ([ValveSoftware/Proton#10113](https://github.com/ValveSoftware/Proton/issues/10113)):
  *"Proton runs your real driver to the lighthouse heartbeat; here is the precise gap;
  please enable a Proton/usermode tier for WARDOGS as for The Finals."*
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
