---
id: TASK-8
title: Vendor Elytra patch series + author AGENTS.md
status: Done
assignee: []
created_date: '2026-09-20 04:43'
updated_date: '2026-09-20 04:43'
labels:
  - docs
milestone: m-0
dependencies: []
priority: high
ordinal: 8000
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Vendor AstralDrift's 3-patch Elytra series with provenance; write AGENTS.md covering dendritic/clean-code rules, Proton repo map, build/debug/packaging workflow, and the attestation ceiling.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 3 patches vendored under nix/wine-patches with provenance README
- [x] #2 patches apply cleanly in series to dc26e6184
- [x] #3 AGENTS.md documents rules, repo map, workflow, ethics/ceiling
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Series 0001-0003 (NtLoadDriver->SCM+RtlStringFromGUIDEx+ksecdd, bcrypt in-place decrypt, MmMapLockedPagesSpecifyCache) vendored + verified applying. AGENTS.md written incl. Embark-FAQ-derived attestation ceiling (Secure Boot/HVCI/DSE/non-VM cannot exist under Wine -> heartbeat vendor-gated).
<!-- SECTION:FINAL_SUMMARY:END -->
