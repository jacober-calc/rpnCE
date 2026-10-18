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

1. Give `CMD_XROOT_Y` a dedicated case in `precheck()`, preserving the intended X=0 -> `ERR_DIV0` behaviour. MATH-019 reports an observed 0 rather than an error, despite the current source guard; investigate this source/build/runtime discrepancy before implementation.
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
- Zero-index handling produces the correct error on hardware; investigate MATH-019's reported 0 and reconcile it with the source precheck.
- Errors preserve stack and LASTX.
- All suites pass via `sh tests/run_tests.sh`, and targeted tests pass on TI-84 Plus CE.
- No change to the current release is made until a separate code-change PR is reviewed and merged.

## Scope

This PR records the verified defect and actionable implementation/test specification **without changing any application code**. The actual fix requires a subsequent implementation PR.
