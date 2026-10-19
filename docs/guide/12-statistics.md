# 12. STAT: statistics and curve fitting

RPNCE 0.7.0 · User Guide

[Manual contents](../USER_GUIDE.md) · [Previous: 11. BASE: hexadecimal, octal and binary](11-base.md) · [Next: 13. TVM: time value of money](13-tvm.md)

**In this chapter**

- [Entering data](#entering-data)
- [Results](#results)
- [Curve fitting](#curve-fitting)
- [ALLΣ and LINΣ](#allς-and-linς)
- [Example: enter a data set and fit a straight line](#example-enter-a-data-set-and-fit-a-straight-line)
- [Choosing a statistic](#choosing-a-statistic)
- [Fit restrictions](#fit-restrictions)

## Entering data

Each data point is a pair (x, y). Enter `y ENTER x`, then press **Σ+** in
the **STAT** menu. X then shows the number of points so far, and the next
number you type replaces it, so entering data is quick. For one-variable
data, just type `x` and press **Σ+**.

To remove a point (to fix a mistake), enter it again and press **Σ−**.
**CLΣ** clears all the data, which is kept between runs.

Example, five points: `3.1 ENTER 1` **Σ+**, `4.9 ENTER 2` **Σ+**,
`7.2 ENTER 3` **Σ+**, `8.8 ENTER 4` **Σ+**, `11.1 ENTER 5` **Σ+**.

## Results

| Item | Gives (for the example data) |
| --- | --- |
| **SUM** | Σx in X and Σy in Y (15 and 35.1) |
| **MEAN** | x̄ in X and ȳ in Y (3 and 7.02) |
| **WMN** | the weighted mean: x weighted by y in X, y weighted by x in Y |
| **N** | the number of points (5) |
| **SDEV** | sample standard deviation of x in X and of y in Y (1.5811 and 3.1507) |
| **PDEV** | population standard deviation |
| **SCOV**, **PCOV** | sample and population covariance |
| **Σ** | a submenu that recalls any of the 13 sums |

## Curve fitting

**CFIT** fits a curve to the data. Choose the model in its row 2:

| Model | Curve |
| --- | --- |
| **LINF** | y = a + bx |
| **LOGF** | y = a + b ln x (needs every x > 0) |
| **EXPF** | y = a e^(bx) (needs every y > 0) |
| **PWRF** | y = a x^b (needs every x > 0 and y > 0) |
| **BEST** | picks the model with the strongest correlation and names it |

Then, in row 1: **SLOP** (b), **YINT** (a), **CORR** (the correlation
coefficient), **X→Y** (the y the curve predicts for the x in X) and
**Y→X** (the x for the y in X). For the example data with **LINF**:
**SLOP** 1.99, **YINT** 1.05, **CORR** 0.99865, and `6` **X→Y** gives 12.99.

## ALLΣ and LINΣ

**ALLΣ** (the default) keeps all 13 sums, which the LOG, EXP and PWR fits
need. **LINΣ** keeps only the six linear sums, as on the HP-32SII; then
only **LINF** is available.


## Example: enter a data set and fit a straight line

Use the following five observations:

| x | y |
| --- | --- |
| 1 | 3.1 |
| 2 | 4.9 |
| 3 | 7.2 |
| 4 | 8.8 |
| 5 | 11.1 |

**Setup:** DEC, FIX 4. Open STAT, choose **CLΣ** to replace the previous
data, and choose **ALLΣ**. Open **CFIT**, select **LINF**, then return to
STAT. This explicitly selects the data mode and fit model rather than
relying on remembered settings.

| Keys | X after Σ+ | Explanation |
| --- | --- | --- |
| `3.1 ENTER 1` **Σ+** | 1 | Enter y first, then x. |
| `4.9 ENTER 2` **Σ+** | 2 | Add the second point. |
| `7.2 ENTER 3` **Σ+** | 3 | Add the third point. |
| `8.8 ENTER 4` **Σ+** | 4 | Add the fourth point. |
| `11.1 ENTER 5` **Σ+** | 5 | The displayed result is the observation count. |

Request **MEAN**: X = 3 and Y = 7.02. In **CFIT**, request **SLOP** for
1.99 and **YINT** for 1.05. Enter `6`, then **X→Y**, to forecast 12.99
from the fitted equation `y = 1.05 + 1.99x`.

To correct the last observation, re-enter `11.1 ENTER 5` and press **Σ−**.
The count becomes 4. Enter the corrected y and x and use **Σ+** again.
Subtracting a point requires its original values: RPNCE keeps accumulated
sums rather than a browsable list of observations.

## Choosing a statistic

**SDEV** and **SCOV** use the sample denominator n − 1, and therefore need
at least two observations. **PDEV** and **PCOV** use n for population
statistics. Standard-deviation functions return the x result in X and
the y result in Y; covariance returns a single result in X.

For **WMN**, X receives Σxy / Σy and Y receives Σxy / Σx. To use y as a
weight on x, enter the weight first, then the measured x value. A zero
weight sum cannot produce the corresponding weighted mean.

The **Σ** submenu exposes all 13 accumulated sums. These can be inspected
without changing the observations. If you edit sums through numbered
registers, keep the [statistics-register rules](08-registers.md#the-statistics-registers)
in mind: manually altered sums can invalidate nonlinear fits.

## Fit restrictions

A model also needs variation in its predictor: data with identical x
values cannot determine a straight-line slope. LOGF needs positive x,
EXPF needs positive y, and PWRF needs both. **BEST** chooses among models
available for the accumulated data; it is a comparison by correlation,
not a guarantee that the selected relationship explains the process.
Use the [error reference](16-troubleshooting.md#errors) if a result reports
`Err: Stat`.

Entering a nonpositive value disables the logarithmic sums that need it.
Removing that observation with Σ− does not re-enable those sums; clear
the statistics and re-enter the corrected data. Switching from LINΣ to
ALLΣ after entering observations likewise cannot reconstruct sums that
were not accumulated. Select ALLΣ before entering a new nonlinear-fit data
set.

---

[Manual contents](../USER_GUIDE.md) · [Previous: 11. BASE: hexadecimal, octal and binary](11-base.md) · [Next: 13. TVM: time value of money](13-tvm.md)
