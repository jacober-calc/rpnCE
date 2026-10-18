# Proposal: XROOT_Y rejects negative radicands with even root indices

**Status:** Documentation-only change proposal; no calculator source or test code is modified by this PR.

## Hardware reproduction (TI-84 Plus CE, RPNCE)

- MATH-010: Y=27, X=3, XROOT_Y -> 3 (pass; operand order and stack drop).
- MATH-011: Y=81, X=4, XROOT_Y -> 3 (pass; positive even root).
- MATH-012: Y=-8, X=3, XROOT_Y -> -2 (pass; negative odd root).
- MATH-013: Y=-16, X=4, XROOT_Y -> 1.4142 with no error (fail).
- MATH-014: Repeat after clearing stack; explicitly verify Y=-16, X=4 before invocation -> 1.4142, no warning (fail reproduced). Post-error stack not inspected.
- MATH-015: Y=-16, X=2, XROOT_Y -> 0 with no reported domain error (fail). Stack after operation not inspected. This confirms the failure extends to the square-root index and can yield different incorrect results for different even indices.
- MATH-017: Y=-8, X=-3, XROOT_Y -> -0.5000 with expected stack drop (pass; valid negative odd index).
- MATH-018: Y=-16, X=-4, XROOT_Y -> 0.3536 without domain error (fail; negative even index). Stack after operation not inspected. The missing guard therefore affects both signs of even integer root indices.
- MATH-019: Y=16, X=0, XROOT_Y -> 0 as reported (fail against expected divide-by-zero error); stack after operation not inspected. **Unresolved:** current source includes a zero-index precheck, so verify operand entry, any transient error message, and installed build version before attributing this to the guard itself.
- MATH-020: After clearing the stack, Y=25 and X=0 were explicitly confirmed before execution; XROOT_Y displayed 0, with no reported error (fail). Stack after operation not inspected. This reproduces MATH-019 with a different positive radicand, making an operand-entry mistake less likely. The source/runtime discrepancy remains unresolved.
- MATH-021: Y=0, X=3, XROOT_Y -> 0 with expected stack drop (pass).
- MATH-022: Y=0, X=-3, XROOT_Y -> displayed -3 (fail against expected divide-by-zero error). Stack not inspected; -3 equals the original X operand, so whether the operation was rejected, skipped, or calculated incorrectly remains unresolved. Explicitly test and handle zero radicand with negative index as an independent boundary condition.
- MATH-023: Y=0, X=-1, XROOT_Y -> displayed -1 (fail against expected divide-by-zero error). Stack not inspected. As in MATH-022, the displayed value matches the original X operand; investigate whether the operation was rejected without visible error or produced an incorrect result. This reproduces the zero-radicand/negative-index issue for the reciprocal index -1.
- MATH-027: Y=-3, X=0.5 -> 9 (pass; reciprocal exponent 2).
- MATH-028: Y=-16, X=1.5 -> 6.3496 (pass; reciprocal exponent 2/3).
- MATH-029: Y=-32, X=2.5 -> 4 (pass; reciprocal exponent 2/5).
- MATH-030: Y=-16, X=0.8 -> 1.3066 rather than domain error (fail; reciprocal exponent 5/4 has even denominator). Stack not inspected. The negative-base domain defect extends to fractional indices, not just even integer indices.
- MATH-031: Y=-16, X=-0.8 -> -0.0221 rather than domain error (fail; reciprocal exponent -5/4 has even denominator). Stack not inspected. This confirms the fractional-index domain failure for negative as well as positive indices.
- Comparison: DM32 reports `INVALID y^x` for the invalid negative-even-root case.

## Mathematical expectation

The real-valued fourth root of -16 does not exist. The operation must produce the calculator's standard domain error, not 1.4142 or any other real-valued result.

## Current source analysis

In `src/core/calc.c`, `precheck()` groups `CMD_XROOT_Y` with division and reciprocal and checks only whether X is zero, returning `ERR_DIV0` for zero. There is no negative-radicand/even-index domain check.

The computation path for `CMD_XROOT_Y` is:

```c
real_t inv = os_RealInv(x);
g_result = os_RealPow(y, &inv);
```

This delegates negative-base fractional exponent handling to TI OS power semantics. The observed 1.4142 (positive fourth-root index), 0 (square-root index), and 0.3536 (negative fourth-root index) outputs are wrong; the precise internal reason for this particular value has not been established.

`do_math()` already calls `compute()` before stack mutation, and on a non-`ERR_NONE` result displays `error_text(err)` while leaving the stack unchanged. Therefore prevalidation is the appropriate place for the proposed fix.

## Proposed implementation (NOT implemented in this PR)

1. Give `CMD_XROOT_Y` a dedicated case in `precheck()`, preserving the intended X=0 -> `ERR_DIV0` behaviour. MATH-019 and MATH-020 both report an observed 0 rather than an error, despite the current source guard; MATH-020's operands were verified before execution. Investigate this source/build/runtime discrepancy before implementation.
2. When Y is negative, validate X as an **integer odd root index** before calling `os_RealPow`. Reject even integer X and noninteger X with `ERR_DOMAIN` (unless a deliberately documented broader real-root convention is implemented).
3. Use existing `is_integer()`, `is_neg()`, and TI real arithmetic helpers; do not convert a large root index to a narrow native integer or use floating-point `fmod` in calculator code. An odd/even test should be designed to work within TI's 14-digit real representation, e.g. using TI real division by two and its fractional part. Verify negative root indices also work (odd negative allowed, even negative rejected).
4. Preserve current valid odd-negative-root behaviour, including sign, and ensure no changes to X, Y, LASTX or stack lift on a rejected operation.
5. Check whether `os_RealPow` is trustworthy for *all* negative odd-index roots, especially negative indices and larger odd indices. If not, calculate the magnitude using a positive base and apply the negative sign explicitly; preserve TI OS error protection.
6. Keep the change scoped to XROOT_Y. Any broader `CMD_POW` negative-base-domain policy should be separately considered.

## Required regression tests (to implement later)

Add host-side tests to `tests/test_calc.c` through `calc_execute`, not only a standalone math helper. The existing `tests/run_tests.sh` already builds and runs the `core` test suite.

| Y | X | Expected | Why |
| ---: | ---: | --- | --- |
| 27 | 3 | 3 | positive odd index |
| 81 | 4 | 3 | positive even index |
| -8 | 3 | -2 | valid negative odd root |
| -16 | 4 | `Err: Domain` | reported defect |
| -16 | 2 | `Err: Domain` | negative square root |
| -32 | 5 | -2 | valid negative fifth root |
| -8 | -3 | -0.5 | negative odd index |
| -16 | -4 | `Err: Domain` | negative even index |
| -8 | 2.5 | `Err: Domain` | noninteger index |
| 16 | 0 | `Err: Divide by 0` (confirm exact existing text) | preserve zero-index guard |
| 0 | 3 | 0 | zero radicand |
| 0 | -3 | division-by-zero/domain per documented convention | zero to negative exponent |

For rejected operations, assert the error text and that X, Y, deeper stack levels, and LASTX are unchanged. Assert normal binary stack drop on successful operations. Verify physical-device behaviour after host tests pass.

## Acceptance criteria

- MATH-013 and MATH-014 no longer return 1.4142, MATH-015 no longer returns 0, and MATH-018 no longer returns 0.3536; all report `Err: Domain`.
- Valid odd negative roots still work.
- Zero-index handling produces the correct error on hardware; investigate the reproducible MATH-019/MATH-020 observed 0 and reconcile it with the source precheck.
- Zero radicand with negative index (MATH-022 and MATH-023) reports division-by-zero and preserves operands, rather than merely displaying the original X operand. Also investigate MATH-022 (Y=0, X=-3 -> displayed -3) and MATH-023 (Y=0, X=-1 -> displayed -1), and ensure zero radicand with negative index reports divide-by-zero while preserving the stack.
- Errors preserve stack and LASTX.
- All suites pass via `sh tests/run_tests.sh`, and targeted tests pass on TI-84 Plus CE.
- No change to the current release is made until a separate code-change PR is reviewed and merged.

## Scope

This PR records the verified defect and actionable implementation/test specification **without changing any application code**. The actual fix requires a subsequent implementation PR.


## Additional fractional-index regression requirement (MATH-030)

For a negative radicand, domain validation must account for the reduced rational form of the reciprocal exponent, rather than simply classifying the index as integer/noninteger. Reject reciprocal powers with even denominators (MATH-030, index 0.8 -> 5/4; MATH-031, index -0.8 -> -5/4), while preserving valid cases with odd denominators (MATH-028 index 1.5 -> 2/3 and MATH-029 index 2.5 -> 2/5) and integer reciprocal powers (MATH-027 index 0.5 -> 2). Floating-point/rational classification requires careful tolerance and range handling; do not implement a blanket negative-radicand guard. Add corresponding hardware and host regression tests before accepting the eventual fix.

## MATH-090 and MATH-091 paired hardware regression

- MATH-090 FAIL: With Y=-81 and X=4, XROOT_Y returned 2.1213 rather than Err: Domain; afterward X=2.1213 and Y=0, with an unexpected binary stack drop. Invalid real fourth roots must instead reject the operation and preserve operands and stack.
- MATH-091 PASS: With Y=81 and X=4, XROOT_Y returned X=3 and Y=0, with normal binary stack drop. This positive-radicand control confirms that the same index calculates correctly on a valid input.
- Add both cases to the eventual host and hardware regression tests, checking the error and full stack preservation for MATH-090 and normal result/stack drop for MATH-091. The exact cause of the unexpected 2.1213 remains undetermined.

- MATH-092 PASS: With Y=-243 and X=5, XROOT_Y returned X=-3 and Y=0 with no error and normal binary stack drop. Include this valid negative odd-index case in eventual host and hardware regression tests to guard against overbroad domain rejection.

- MATH-093 FAIL: With Y=-64 and X=6, XROOT_Y returned 1.7321 instead of Err: Domain. Afterward X=1.7321 and Y=0, with an unexpected binary stack drop. This extends the reproduced invalid negative even-root behaviour to sixth roots (in addition to second and fourth roots). Add host/device regression asserting domain error and preservation of operands, deeper stack, and LASTX. The source of 1.7321 is not established.

- MATH-094 PASS (positive sixth-root control): With Y=64 and X=6, XROOT_Y returned X=2 and Y=0, with no error and normal binary stack drop. Paired with MATH-093 (Y=-64, X=6), this confirms valid positive sixth-root computation while negative sixth-root domain rejection fails. Include the positive control in host and hardware regression coverage.

- MATH-095 FAIL: Y=-81, X=-4, XROOT_Y returned X=0.2357 and Y=0 with an unexpected binary stack drop, rather than Err: Domain with X=-4 and Y=-81 unchanged. This independently reproduces the negative-radicand/even-index domain defect for a negative fourth-root index (compare MATH-018). Add a host/device regression asserting domain rejection and preservation of all stack levels and LASTX. The origin of the numerical result is not established.

- MATH-096 PASS (positive reciprocal fourth-root control): With Y=81 and X=-4, XROOT_Y returned X=0.3333 and Y=0, no error, with normal binary stack drop. Paired with MATH-095 (Y=-81, X=-4), this verifies the negative index itself is supported on valid positive radicands. Add to host/device regression coverage to prevent an overbroad negative-index guard.

- MATH-097 PASS (negative reciprocal fifth-root control): With Y=-243 and X=-5, XROOT_Y returned X=-0.3333 and Y=0, no error, with normal binary stack drop. This is a valid negative radicand with negative odd root index; include in host/device regression coverage to ensure the eventual domain guard preserves this behaviour (paired with MATH-092, X=5).

- MATH-098 PASS (fractional-index regression control): With Y=-32 and X=2.5, XROOT_Y returned X=4 and Y=0 with no error and normal binary stack drop, reproducing MATH-029. Reciprocal exponent 1/2.5 = 2/5 gives a valid real result for the negative radicand. Retain this case in eventual host/device regression tests; do not reject all noninteger root indices indiscriminately.

- MATH-099 FAIL (invalid fractional-index regression): With Y=-81 and X=0.8, XROOT_Y returned X=-171.8269 and Y=0 with unexpected binary stack drop, rather than Err: Domain with X=0.8 and Y=-81 preserved. The reciprocal exponent 5/4 has even denominator, so the result is not real. This reproduces the fractional-index domain defect previously seen in MATH-030 at a different radicand magnitude. Add host/device regression asserting domain rejection and preservation of all stack levels and LASTX. Contrast with valid fractional-index MATH-098 (Y=-32, X=2.5, exponent 2/5).

- MATH-100 PASS (positive fractional-index control): With Y=81 and X=0.8, XROOT_Y returned X=243 and Y=0, no error, with normal binary stack drop. This pairs with MATH-099 (Y=-81, X=0.8, invalid domain), confirming the positive-base calculation for reciprocal exponent 5/4 works correctly. Include in eventual host/device regression coverage.

- MATH-101 PASS (zero-index retest): After clearing the stack and explicitly confirming Y=16, X=0, XROOT_Y displayed `Err: Division by 0`; afterward X=0 and Y=16 remained unchanged, with expected error/stack behaviour. This contradicts the observed 0 in MATH-019 (same operands) and MATH-020 (Y=25, X=0). Keep those earlier failures in the historical record but treat the zero-index issue as **not consistently reproducible**, rather than a confirmed persistent defect. Source already contains an X=0 precheck. Re-test MATH-020's exact operands and record build/version and error display if discrepancy recurs. No calculator code changed.

- MATH-102 PASS (second zero-index retest): With stack cleared and operands explicitly confirmed Y=25, X=0, XROOT_Y displayed `Err: Division by 0`; afterward X=0 and Y=25 remained unchanged. This directly contradicts MATH-020 with identical inputs. Together with MATH-101 (Y=16, X=0, PASS), two consecutive hardware retests confirm zero-index error signalling and operand preservation on the currently tested build. Preserve MATH-019/MATH-020 as historical FAIL observations but classify the discrepancy as unresolved/not currently reproducible; do not claim a consistently failing zero-index guard. Investigate version, input sequence, and error presentation if it recurs.

- MATH-103 PASS for invalid-operation rejection and stack preservation (error-category discrepancy): With Y=0 and X=-3 explicitly confirmed, XROOT_Y displayed `Err: Domain` and preserved X=-3 and Y=0, with no stack drop. The test anticipated division-by-zero wording, but domain error is a defensible category for this undefined real-valued operation; decide and document a consistent error convention. This contrasts with MATH-022's earlier display of -3 without an established error observation. Keep MATH-022 as a historical FAIL, but its apparent failure is not currently reproduced. Add a regression for rejecting zero radicand with negative index and preserving the stack, and separately assert the selected error category.
