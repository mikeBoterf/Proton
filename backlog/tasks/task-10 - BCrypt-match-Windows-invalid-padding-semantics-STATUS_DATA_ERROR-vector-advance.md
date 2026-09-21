---
id: TASK-10
title: >-
  BCrypt: match Windows invalid-padding semantics (STATUS_DATA_ERROR + vector
  advance)
status: Done
assignee: []
created_date: '2026-09-21 04:12'
updated_date: '2026-09-21 04:20'
labels:
  - wine
  - bcrypt
  - conformance
milestone: m-1
dependencies: []
modified_files:
  - wine/dlls/bcrypt/bcrypt_main.c
  - wine/dlls/bcrypt/tests/bcrypt.c
  - nix/wine-patches/0005-bcrypt-Match-Windows-invalid-padding-semantics.patch
priority: high
type: bug
ordinal: 10000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Wine's padded symmetric decrypt diverges from Windows on rejected PKCS7 padding in three measured ways. Windows baseline measured on build 26200.9457 (docs/wardogs-elytra-independent-review.md, windows-results package), reproduced 16/16 across zero-length / over-long / inconsistent padding, default and Microsoft Primitive Provider, in-place and separate buffers.

Divergences:
1. Status: Wine returns STATUS_UNSUCCESSFUL (0xc0000001), Windows returns STATUS_DATA_ERROR (0xc000003e). Upstream Wine carried a literal `FIXME: invalid padding` on this branch; patch 0002 inherited it.
2. Vector: Windows advances the IV to the final ciphertext block even on rejection. Wine's failure path `goto done` jumps past the vector update entirely.
3. pcbResult: Windows reports input_len minus the claimed padding length (48/31/46 for the three cases); Wine leaves it at input_len. Recorded by the Windows package as an observation, not a documented contract.

NOT in scope: changing whether invalid padding is rejected. Both platforms reject; only the reported semantics differ.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Both invalid-padding branches return STATUS_DATA_ERROR
- [x] #2 Vector/IV advances to final ciphertext block on rejection, matching Windows
- [x] #3 pcbResult matches measured Windows values, with underflow guard
- [x] #4 In-tree regression test added to dlls/bcrypt/tests/bcrypt.c
- [x] #5 padding-matrix reports 16/16 against the patched build
- [x] #6 Patch 0002 regenerated from the submodule
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Implemented as a separate patch 0005 rather than editing the vendored 0002, so the conformance change stays isolated and independently upstreamable (TASK-14).

Changes in wine/dlls/bcrypt/bcrypt_main.c:
- Both invalid-padding branches now return STATUS_DATA_ERROR instead of STATUS_UNSUCCESSFUL, resolving the FIXME upstream Wine carried on this branch.
- Padding failures now jump to a new `update_vector` label instead of `done`, so the vector/IV advances to the final ciphertext block on rejection as Windows does. The existing `trailing_blocks` capture already preserved what was needed; the failure path was simply jumping past it.
- *ret_len is reduced by the claimed padding length on rejection, with an underflow guard, matching the measured 48/31/46. STATUS_BUFFER_TOO_SMALL deliberately still skips the vector update — that case was not measured on Windows.

Added an in-tree regression test to dlls/bcrypt/tests/bcrypt.c covering the rejected status and the IV state.

VERIFIED by build and run, not by inspection. Built bcrypt.dll from the submodule (nix devshell, --enable-archs=x86_64) and ran against a hardlinked copy of the compat tool:

  padding-matrix  baseline 4/16 PASS  ->  patched 16/16 PASS
  status          0xc0000001          ->  0xc000003e   (Windows: 0xc000003e)
  pcbResult       48/48/48            ->  48/31/46     (Windows: 48/31/46)
  IV advanced     no                  ->  yes          (Windows: yes)

Captured runs in docs/elytra-diagnostics/results/.

The IV divergence was not in the original analysis from either investigation — it was found by reading the padded-decrypt path against the Windows matrix, then confirmed empirically (baseline reports final_iv_matches=0 on all twelve invalid cases).

NOT VERIFIED: whether this changes anything for WARDOGS. See TASK-12 — on Windows the retail decrypt succeeds, so Windows never reaches this branch at all.
<!-- SECTION:FINAL_SUMMARY:END -->
