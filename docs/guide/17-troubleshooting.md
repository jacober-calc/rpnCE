# 17. Messages and errors

rpnCE development build · User Guide

[Manual contents](../USER_GUIDE.md) · [Previous: 16. DATE: dates, times and the clock](16-date.md) · [Next: 18. Saving, memory and files](18-memory.md)

**In this chapter**

- [Errors](#errors)
- [Notices](#notices)
- [BASE warnings](#base-warnings)
- [When rpnCE can't start](#when-rpnce-cant-start)
- [Choosing the next action](#choosing-the-next-action)
- [Recovering without clearing saved work](#recovering-without-clearing-saved-work)
- [Reporting a reproducible problem](#reporting-a-reproducible-problem)

Errors appear on the message line until a key is pressed. `CLEAR` dismisses
the message without changing the stack or an unfinished entry. Other keys
dismiss the message and perform their normal action. Routine notices fade
after about 1.5 seconds. A rejected arithmetic operation retains its operands
and LASTX, so you can inspect the inputs before trying again. An invalid
DATE or complex entry remains open for correction with DEL; it is not a
completed stack value.

<img src="../../assets/guide/screens/trouble-domain.png" alt="Err: Domain after 2 (−) 2ND √, with −2 still in X" width="320"> <img src="../../assets/guide/screens/trouble-invalid.png" alt="Err: Invalid, with the DATE entry still open" width="320">

## Errors

| Message | Meaning |
| --- | --- |
| Err: Division By 0 | a division by zero: `÷` or `x⁻¹` of 0, **MOD** by 0, **%CH** from 0, a root with index 0 |
| Err: Domain | an input the function doesn't accept: a negative real √ or LN in RRES, ASIN of 2, (−8)^0.4 in RRES, a size out of range, and so on |
| Err: Overflow | a result beyond the calculator's range (about ±9.99 × 10⁹⁹) |
| Err: Underflow | a nonzero result or required intermediate value is too small for the supported range |
| Err: Precision | detected loss of significant digits makes the calculation unreliable |
| Err: Math | another math error reported by the calculator |
| Err: Stat | not enough data for a statistic, or a curve fit that doesn't apply to the data |
| Err: Data Type | an unsupported value kind or combination: complex input to a real-only function, a DATE object where a number is needed, or unsupported quantity arithmetic |
| Err: Unit Mismatch | units of different kinds, such as metres and kilograms |
| Err: Index | `(i)` names a register that doesn't exist |
| Err: Word Size | a BASE operation on a value too big for the word size |
| Err: Invalid | an impossible DATE record (February 30, 23:59:60) |
| Err: Syntax | number, complex or DATE entry that can't be read |
| Err: Clock | the CE's clock can't be read |
| TVM Not Found | no interest rate fits the TVM values |
| Not available yet | a menu action without an implementation |

## Notices

| Message | After |
| --- | --- |
| TVM Stored, TVM Calculated | storing or solving a TVM value |
| TVM Cleared, TVM Reset | **CLTV**, **RSTV** |
| Stack cleared, Registers Cleared, Stats Cleared | `CLEAR CLEAR` or **CLST**, **CLRG**, **CLΣ** |
| Stack Size Set, Register Size Set, Word Size Set | **SSIZ**, **RSIZ**, **WSIZ** |
| Time Zone Set, Clock Set, Epoch Set | **ATZ** or **CTZ**, **SETC**, **EPC** |
| VARS | the VARS catalog is open |
| a tint name, such as LCD+ | **TINT** |

## BASE warnings

In HEX, OCT and BIN, the message line explains why X is shown in decimal:
"Exceeds 32-bit word" (with the current word size), "Negative: not a
word", or "Not a whole number". See [chapter 12](12-base.md#12-base-hexadecimal-octal-and-binary).

## When rpnCE can't start

If its resource appvar is missing or doesn't match the program, rpnCE shows
a notice instead of starting: "RPNCEres not found", "RPNCEres is damaged"
or "RPNCEres is the wrong version". Send `rpnCEres-0.7.0.8xv` from the same
release as `rpnCE-0.7.0.8xp`. Local builds omit the version from the filenames.

<img src="../../assets/guide/screens/trouble-no-res.png" alt="The startup notice when RPNCEres is missing" width="320">


## Choosing the next action

| Symptom | Check | Corrective action |
| --- | --- | --- |
| SIN or a polar angle looks wrong | RAD in the status bar; no angle indicator means DEG | Select the intended angle mode before the operation. |
| A complex value's displayed angle and CANG differ | PRAD/PDEG versus DEG/RAD | Complex presentation and CANG have independent angle settings. See [complex modes](09-settings.md#complex-result-and-display-modes). |
| A negative real square root gives Err: Domain | CRES indicator; absent means RRES | Select CRES for supported complex results, or enter an explicit complex operand. |
| A value with imaginary zero gives Err: Data Type | It is still a complex value | Use CPLX REAL if discarding the imaginary component is intended; CRES does not extend real-only functions. |
| Ordinary arithmetic wraps or only whole digits can be entered | HEX/OCT/BIN in the status bar | Open BASE and select DEC. |
| A result looks rounded but later arithmetic differs | FIX/SCI/ENG display setting | Use SHOW to inspect it; use RNDF or RNDN only if the stored value should be rounded. |
| A STO/RCL key opens the wrong operation | A prompt is waiting for a letter or operator | Cancel with CLEAR, DEL, or ON; repeat the complete command. Use ALPHA for W, R, or M where needed. |
| A TVM softkey stores instead of solving | No TVM variable key immediately preceded it | Recall a known TVM variable with 2ND plus its key, then select the unknown variable. |
| TVM result has an unexpected sign or amount | Cash-flow signs, N, P/YR, C/YR, BEG/END | Enter all assumptions explicitly; N counts payments, not years. |
| A curve fit reports Err: Stat | Count, variation, ALLΣ/LINΣ, and positive-data requirements | Select the correct mode, clear the sums, and re-enter valid data if necessary. |
| A unit operation fails | Operand kinds and TEMP/FUEL restrictions | Use compatible quantities; review the operation table in the units chapter. |
| A quantity imported from a version-5 backup has lost its unit | Its old RPNCEunt metadata was absent, damaged, or mismatched | Restore a matched old backup or reattach the verified unit. Current saves keep units with their values. |
| No program starts from PRGM | Native-program support for this OS | Follow the launcher's current instructions; sending files alone does not enable native code. |

## Recovering without clearing saved work

First inspect the state: SHOW reveals the stored X; VARS reveals registers
and shared TVM values; the status bar reveals base, angles, and number
format. Returning to DEC or selecting FIX does not erase ordinary values.
Use the narrowest clear operation needed: CLX for X, CLΣ for statistics,
CLTV for a new finance problem. CLRG does not clear statistics or TVM.

If startup reports a resource problem, replace the matching program and
resource files first. Deleting saved-data appvars does not repair a
mismatched resource. Before an intentional reset, follow the
[backup procedure](18-memory.md#backing-up-your-work).

## Reporting a reproducible problem

Include the rpnCE version, calculator model and OS, starting settings,
exact keys in order, expected result, and displayed result or error. For
units, include the full menu path: kW under PWR and kWh under ENER are not
interchangeable. For TVM, include all seven values and BEG/END; for BASE,
include word size and carry if relevant.

Report problems through the [project issue tracker](https://github.com/jacober-calc/rpnCE/issues).
A short sequence from known settings is more useful than an isolated
screen value. Avoid resetting a problem state before recording it if it
contains the only copy of the inputs.

---

[Manual contents](../USER_GUIDE.md) · [Previous: 16. DATE: dates, times and the clock](16-date.md) · [Next: 18. Saving, memory and files](18-memory.md)
