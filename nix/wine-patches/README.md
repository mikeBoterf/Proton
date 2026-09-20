# Wine patches for WARDOGS / Elytra compatibility

Vendored from AstralDrift's research build, which targets Valve Wine `proton_11.0`
and applies cleanly onto commit `dc26e6184` — the exact commit this repo's `wine`
submodule is pinned to (verified 2026-09-20).

Source: https://github.com/AstralDrift/Proton-WARDOGS-Elytra (patches/)
Wine branch: https://github.com/AstralDrift/wine/tree/wardogs-elytra-proton-compat

- 0001 — real NtLoadDriver (→ SCM → winedevice), RtlStringFromGUIDEx, ksecdd BCrypt
         forwarding, IRP security context / create disposition.
- 0002 — padded in-place symmetric decryption fix (encrypted lighthouse driver).
- 0003 — MmMapLockedPagesSpecifyCache.

These are compatibility fixes that return REAL driver/crypto results — not an
anti-cheat bypass. They advance the client to the lighthouse session-prime /
heartbeat wall, which is vendor-gated (Embark must enable Elytra Linux support for
WARDOGS, as they already have for THE FINALS and Arc Raiders).

## Nix build bootstrap

`nix build .#wine-proton` builds Valve's *git* Wine, which strips generated files
that release tarballs (what nixpkgs' recipe expects) ship. `nix/wine-proton.nix`
regenerates them offline in `postPatch`, in order:

1. `perl tools/make_requests`  → `include/wine/server_protocol.h`, `server/request_*.h`
2. `perl tools/make_specfiles` → `dlls/ntdll/ntsyscalls.h`, `dlls/win32u/win32syscalls.h`
3. `HOME=$TMPDIR python3 dlls/winevulkan/make_vulkan -x vk.xml -X video.xml`
   → `include/wine/vulkan.h` (in-tree xml avoids the network; writable HOME for its
   unconditional cache `makedirs`)
4. `autoconf` → `configure`;  `autoheader` → `include/config.h.in`

Plus `configureFlags += --without-ffmpeg`: `winedmo` vendors FFmpeg source that
doesn't compile against nixpkgs' FFmpeg 7.x headers, and it's in-game media
playback — irrelevant to the Elytra driver/session path.

Verified 2026-09-20: the resulting dist exports `RtlStringFromGUIDEx`, carries the
real `NtLoadDriver` (SCM → winedevice) in `ntdll.dll`, and builds `bcrypt.dll` /
`ntoskrnl.exe` with 0002/0003. See AGENTS.md §8 for the attestation ceiling — a
built driver that *loads* still cannot produce a *passing* heartbeat under Wine.

TODO (our value-add, not in this series):
- a real `\??\Volume{GUID}` resolver in Wine (upstream), replacing the dosdevices
  symlink workaround.
