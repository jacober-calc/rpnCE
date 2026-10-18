# RPNCE TI-84 Plus CE — MATH submenu manual QA report

**Status:** Manual MATH submenu campaign CLOSED after MATH-106. **Documentation only.** This is an evidence and handoff document, not a code fix or automated test run.

## Campaign protocol
- Device: TI-84 Plus CE running RPNCE; operator manually enters values and reports screen observations.
- Test identifiers: MATH-001 through MATH-106 executed; MATH-107 is reserved for optional post-fix verification, not executed.
- For each test: clear stack as specified, enter X (unary) or Y then X (binary), navigate `MATH → MATH` to the function, compare the displayed result/error with the real-number expectation, and where observed inspect X/Y and stack drop/preservation.
- Normal binary success consumes X/Y and produces the result in X; errors should leave operands and deeper stack untouched. Earlier tests did not always inspect stack, so do not infer stack behaviour when not recorded.
- Displays are commonly rounded to four decimal places; numerical comparisons are made at that precision unless otherwise stated. Angle mode matters for ATN2.
- Counts are **historical outcomes, not unique confirmed defects**. Repeated failures and later passing retests both count as separate executions.

## Final execution totals (through MATH-106)
**106 executed: 89 PASS, 16 FAIL, 1 provisional.** MATH-107 not executed.

## Function-level disposition

| MATH function | Evidence | Disposition |
|---|---|---|
| X³ | MATH-001 onward | Passed tested cases |
| ∛X | MATH-001–009 group | Passed tested cases |
| X√Y (XROOT_Y) | MATH-010–032, MATH-090–105 | **Confirmed negative-base domain defect**; other earlier anomalies not reproduced on retest |
| ATN2 | MATH-033–043 | Quadrants, axes and radians passed; MATH-041 origin result 0 provisional because atan2(0,0) has no unique mathematical angle |
| 2^X | MATH-044–048 | Passed |
| LOG2 | MATH-049–054 | Passed including domain rejection |
| LOGB | MATH-055–062 | Passed including error handling; note division-by-zero error text |
| E^X− | MATH-063–078 | Most passed; MATH-068 and 069 were anomalous failures not reproduced by later related tests |
| LN1+ | MATH-079–089 | Passed domain, near-zero, boundary, large-input and inverse checks |

## Confirmed XROOT_Y defect: invalid negative radicands
The calculator returns real numbers instead of a domain error for some invalid real roots of negative radicands. Some later tests explicitly show the binary stack drops on these invalid operations. These are **not** merely rounding errors.

| Test | Y | X | Expected | Observed |
|---|---:|---:|---|---|
| MATH-013/014 | -16 | 4 | Err: Domain | 1.4142 |
| MATH-015 | -16 | 2 | Err: Domain | 0 |
| MATH-018 | -16 | -4 | Err: Domain | 0.3536 |
| MATH-030 | -16 | 0.8 | Err: Domain | 1.3066 |
| MATH-031 | -16 | -0.8 | Err: Domain | -0.0221 |
| MATH-090 | -81 | 4 | Err: Domain | 2.1213; X=2.1213, Y=0, unexpected stack drop |
| MATH-093 | -64 | 6 | Err: Domain | 1.7321; X=1.7321, Y=0, unexpected stack drop |
| MATH-095 | -81 | -4 | Err: Domain | 0.2357; X=0.2357, Y=0, unexpected stack drop |
| MATH-099 | -81 | 0.8 | Err: Domain | -171.8269; X=-171.8269, Y=0, unexpected stack drop |

The rational reciprocal exponent matters: for X=0.8, 1/X=5/4, invalid on a negative base in the reals; for X=2.5, 1/X=2/5, valid. A simplistic “reject all noninteger X” guard would break valid functionality.

## Positive controls: must keep working
- MATH-091: Y=81, X=4 → 3, Y=0, normal stack drop.
- MATH-092: Y=-243, X=5 → -3, Y=0, normal stack drop.
- MATH-094: Y=64, X=6 → 2, Y=0, normal stack drop.
- MATH-096: Y=81, X=-4 → 0.3333, Y=0, normal stack drop.
- MATH-097: Y=-243, X=-5 → -0.3333, Y=0, normal stack drop.
- MATH-098: Y=-32, X=2.5 → 4, Y=0, normal stack drop.
- MATH-100: Y=81, X=0.8 → 243, Y=0, normal stack drop.
- MATH-105: Y=0, X=5 → 0, Y=0, normal stack drop.
- MATH-106: Y=16, X=-0.5 → 0.0039 (exact 1/256), Y=0, no error, normal binary stack drop; confirms valid negative fractional root index.
- Earlier controls include MATH-010/011/012/016/017/021/024–029.

## Historical failures that did not reproduce
- MATH-019 (Y=16, X=0) and MATH-020 (Y=25, X=0) originally displayed 0 and were recorded FAIL. MATH-101/102 retested the same operands and both returned `Err: Division by 0` with X/Y preserved. Keep original failures in totals but **do not claim a persistent zero-index defect**.
- MATH-022 (Y=0, X=-3) and MATH-023 (Y=0, X=-1) originally displayed unchanged X without an established error observation; recorded FAIL. MATH-103/104 retested both and returned `Err: Domain` with X/Y preserved. Treat as historical ambiguous observations, not currently reproduced. The originally expected division-by-zero wording differs from the observed domain-error category; specify the intended convention.
- MATH-068/069: after E^X−(0.0001), multiplying by 10000 yielded anomalous results 1.7186 and 0.1000, respectively. MATH-074 repeated the operation and yielded 1.0001 as expected; MATH-070–076 and other E^X− tests passed. Do not call the anomalies a confirmed reproducible function defect without further evidence.

## Provisional case
MATH-041 ATN2 Y=0, X=0 produced 0; mathematical atan2(0,0) is undefined. Document the chosen application convention before marking definitive PASS/FAIL.

## Code investigation and follow-up agent instructions
1. **Do not change application code in this documentation PR.** This report records observations and requirements only. Do not merge as a fix.
2. Inspect `src/core/calc.c` `precheck()`, `compute()`, and `do_math()`; the existing proposal records that XROOT_Y computes `os_RealPow(y, os_RealInv(x))` and has an X=0 precheck. Verify against the current source before editing.
3. Plan a **separate implementation PR** to enforce correct real-number domain classification for negative bases, including rational reciprocal exponents. Be careful with TI's real representation and numeric tolerance; no broad integer-only rejection.
4. On invalid operations, return a documented error and **do not mutate X, Y, deeper stack levels, LASTX or stack lift**. Existing hardware observations directly confirm X/Y drops; preservation of deeper stack and LASTX is a required future assertion, not something established by the manual tests.
5. Preserve all valid controls listed above, including negative odd indices, negative fractional indices where real, positive bases and zero radicands with positive indices.
6. Add host tests via `calc_execute` in `tests/test_calc.c`, run `sh tests/run_tests.sh`, and repeat targeted cases on hardware. Neither automated testing nor a fix has been performed by this documentation PR.
7. Investigate the zero-index and E^X− historical anomalies only if they recur; keep them distinct from the confirmed negative-base domain defect.
8. Do not assume the strange returned values reveal TI OS's internal calculation. Root cause of those specific numbers is unproven.

## Source-of-truth and closure
The historical per-case observations and proposed implementation discussion are preserved in [the XROOT_Y proposal](xroot-negative-even-domain.md). The PR description should link both documents. MATH-106 passed with X=0.0039, Y=0 and normal stack behaviour; the campaign is **closed** at 106 tests. MATH-107 is not part of this campaign and may be assigned as a post-fix regression test.
