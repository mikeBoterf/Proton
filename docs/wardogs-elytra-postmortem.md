# WARDOGS / Elytra investigation — what we got wrong

**A method postmortem, not a restatement of the findings.** For the technical
account see [`wardogs-elytra-proton-findings.md`](wardogs-elytra-proton-findings.md)
and the second reproduction in
[`wardogs-elytra-independent-review.md`](wardogs-elytra-independent-review.md).
A dated ledger of every claim and its fate is in
[`wardogs-elytra-timeline.md`](wardogs-elytra-timeline.md).

This branch published a confident root cause that did not survive contact with a
second investigator. The mechanism is documented elsewhere; this file records how a
wrong conclusion got as far as a public write-up, so the next person recognises the
pattern earlier.

---

## 1. An argument from elimination was presented as proof

The original conclusion — that `lighthouse` performs a direct kernel-authenticity
check Wine cannot satisfy — was never observed. It was what remained after ruling
other things out, and it was written up as established fact. The backlog entry even
says so in as many words: *"Proven by elimination, not assumed."* Elimination is only
as strong as the completeness of the list and the quality of each exclusion. Both
were weaker than claimed.

The tell was available at the time: the conclusion was unfalsifiable with the tools
in use. "The driver checks the kernel and Wine has no kernel" predicts exactly the
same observable — `STATUS_UNSUCCESSFUL` — as a dozen other failures. A hypothesis
that cannot be distinguished from its rivals by any measurement we can take is not a
finding.

## 2. The tests that "ruled out" crypto could not have detected the bug

`elytra-diagnostics/bcrypt_inplace_test.c` was cited as proving Wine's AES
"bit-for-bit correct". It cannot show that:

- It encrypts **and** decrypts through the same Wine backend (lines 43 and 51), then
  compares. That establishes self-consistency, not equivalence to Windows. An
  implementation wrong in a symmetric way passes cleanly.
- Both calls pass `dwFlags = 0` — so the padded path, the one that reportedly fails,
  was never exercised at all.
- `main()` returns `0` unconditionally and discards every `test_mode` result
  (line 72). The exit code was structurally incapable of reporting failure.

Three independent defects, all in the direction of producing a pass. A test that can
only pass is not evidence.

**What the exit-code bug was actually hiding.** After making the exit code meaningful,
the very first baseline run reported `[AES-ECB 1MB] encrypt fail 0xc000000d`. The ECB
case had been failing for the entire investigation and nobody knew, because `main()`
returned `0` regardless. (It turned out to be a defect in the test — it passed an IV
for ECB, which Wine and Windows both reject — not a Wine bug. That is not the point.
The point is that a broken case sat in a suite cited as proof, invisible, for the
whole investigation.)

## 3. The summary claimed more than the body supported

The findings document opened by saying the patched stack ran "all the way to the
server heartbeat" with a ~60-second live match. Its own detail sections said the
normal launch stops at `WD-L014`, and that the ~60s session came from a
**launcher-bypass** path which *"does not exercise these patches"*. The summary and
the body contradicted each other, and the summary is what a reader takes away.

## 4. The correct answer was reached first, then argued away

The backlog record shows the `+bcrypt` trace entry concluding **"ROOT CAUSE likely
category (a) FIXABLE … Wine produced wrong plaintext"** — essentially the reading the
second investigation now favours. It was abandoned in the next entry on the strength
of the crypto tests described in §2.

This is the most uncomfortable item here. The investigation had the right instinct,
then talked itself out of it using tests that could not have contradicted it. Where a
record shows a hypothesis being dropped, the thing to audit is the evidence that
displaced it.

## 5. Cited artifacts were not preserved

The second reproduction references `generate-fixtures.py`, `crypto-probe.c`,
`run-crypto-probe.sh` and a `logs/` tree. None are in this repository. Its central
claims — the relay trace showing `BCryptDecrypt` returning `0xc0000001`, the
diagnostic BCrypt build identifying invalid PKCS7 padding, the LibTomCrypt
cross-check — rest on evidence a reader cannot inspect.

That is the same weakness this postmortem criticises in §2, pointed the other way.
Both investigations should be held to it. The fixtures in `elytra-diagnostics/fixtures/`
were regenerated with openssl for exactly this reason: an external implementation,
reproducible from a committed recipe.

## 6. One divergence nobody had looked for

Re-reading the padded-decrypt path against the Windows matrix turned up a third
difference that neither investigation had noticed: on rejected padding Windows still
**advances the IV** to the final ciphertext block, while Wine's failure path jumps
past the vector update entirely. It was found by reading code against a measurement,
and then confirmed empirically — the baseline run reports `final_iv_matches=0` on all
twelve invalid cases where Windows measured true.

Worth noting what surfaced it: a concrete measurement to compare against. Neither
investigation found it while reasoning about the failure in the abstract.

---

## 7. How it was finally settled — and what that cost

The question the two documents contradicted each other over was *"what `dwFlags` does
the retail driver pass to `BCryptDecrypt`?"* It was answered by running the game with
`WINEDEBUG=+bcrypt` and reading one line:

```
BCryptDecrypt 0xC221E0, 0xD20050, 55275056, (null), 0xC1F820, 16,
              0xD20050, 55275056, 0xC1F75C, 0x1
```

`dwFlags = 0x1` = `BCRYPT_BLOCK_PADDING`. The main findings document was wrong; the
second investigation was right.

**`BCryptDecrypt` has printed that value since long before this investigation
started.** `bcrypt_main.c:2462` traces the full parameter list. No diagnostic build,
no instrumentation, no reverse engineering — one environment variable and a `grep`.

The same trace showed the driver's entire thread ending at the decrypt with no
signature verification after it, which eliminated the attestation reading outright.
Also free.

Two days of argument, a retracted public claim, and a second investigator were spent
on a question a standard debug channel answers in thirty seconds. The lesson is not
"use `+bcrypt`" — it is that **the cheapest direct observation should be exhausted
before any argument from elimination is even started.** The order was backwards:
elimination first, measurement last.

## What is still unknown

- **Why the decrypt fails.** No root cause established. Candidates: wrong key
  material, wrong IV, buffer content differing from Windows, or a Wine divergence in
  the padded 52.7 MB path that 48-byte fixtures do not reach. This is
  [TASK-12](../backlog/tasks).
- **`BCryptGenerateSymmetricKey ignoring object buffer`** — a real Wine FIXME sitting
  on the exact failing sequence. The driver supplies a 654-byte key object that Wine
  discards for its own allocation. Never explained, flagged twice, still unexamined.
- **Whether `lighthouse` performs environment attestation at all.** Eliminated as the
  *proximate* cause, since the driver never reaches it. Not disproven as a later gate.
- **A native Windows kernel BCrypt/IOCTL trace.** Would show what the same call looks
  like when it succeeds. Still not obtainable with the tooling tried so far.

## What we tried afterwards, and how far it got

The conformance divergence was fixed and **verified by running it**, not by reading
the diff: `bcrypt.dll` was rebuilt from the submodule and exercised against a
hardlinked copy of the compat tool. `padding-matrix` went from 4/16 to **16/16**,
matching the measured Windows semantics on status, `pcbResult` and IV advance. The
captured before/after runs are in `elytra-diagnostics/results/`.

**The game itself was not launched with the patched build.** Installing the rebuilt
DLL into the live Steam compatibility tool was blocked as an unsafe modification of
the user's Steam installation, so the end-to-end attempt stops here pending an
explicit decision. The original tool is byte-for-byte unmodified; a patched copy and a
backup of the original DLLs are staged under `~/.cache/wardogs-padding-test/`.

Worth being clear that a successful launch was never the expected outcome. On Windows
the retail decrypt returns `0x00000000` — Windows never reaches the branch whose
status Wine was reporting wrongly. Fixing it was worth doing because it is a real
conformance bug with a measured baseline and a regression test, not because it was
likely to move WD-L014.

## What changed as a result

The findings document now presents two competing readings instead of a conclusion.
The diagnostics README states what each test does and does not establish. The backlog
verdict is retracted with the reasoning preserved rather than edited away. Nothing has
been reported upstream, because there is still no root cause to report.

The BCrypt conformance fix that came out of this is being upstreamed on its own
merits — a measured Windows divergence with a regression test — and explicitly *not*
as a WARDOGS fix, since it very likely is not one.

## The process point

A single investigator with agreeable tooling produced four mutually reinforcing
errors and a publishable write-up. What caught it was not better analysis; it was a
second party with different priors, different hardware, and no stake in the first
conclusion. Budget for that earlier than feels necessary.
