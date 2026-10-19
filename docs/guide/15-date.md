# 15. DATE: dates, times and the clock

RPNCE 0.7.0 · User Guide

[Manual contents](../USER_GUIDE.md) · [Previous: 14. UNIT: unit conversions](14-units.md) · [Next: 16. Messages and errors](16-troubleshooting.md)

**In this chapter**

- [DATE objects](#date-objects)
- [Typing DATE objects](#typing-date-objects)
- [How DATE objects are shown](#how-date-objects-are-shown)
- [Arithmetic with dates](#arithmetic-with-dates)
- [Examples](#examples)
- [The DATE submenus](#the-date-submenus)
- [Epochs](#epochs)
- [The clock and time zones](#the-clock-and-time-zones)
- [Procedure: enter a date and count days](#procedure-enter-a-date-and-count-days)
- [Splitting and joining objects](#splitting-and-joining-objects)
- [Epoch conversion details](#epoch-conversion-details)
- [Clock operations that change settings](#clock-operations-that-change-settings)

## DATE objects

The **DATE** menu (HOME row 3) works with seven kinds of **DATE objects**:
values that hold a date, a time or a duration in a single stack level.
They go on the stack and in registers just like numbers, and they're saved
when you quit. Dates use the Gregorian calendar from year 1 to 9999, with
time zones as fixed offsets from UTC.

| Object | Typed as | Shown in ".." mode |
| --- | --- | --- |
| Date | `{2024,3,14` | `2024-03-14` |
| Time | `T{15,36,1` | `15:36:01` |
| DateTime | `{2024,3,14,15,36,1` | `2024-03-14 15:36:01` |
| TimeZone | `{-7,0` | `UTC-07:00` |
| ZonedDateTime | `{2024,3,14,15,36,1,-7,0` | `2024-03-14 15:36:01-07:00` |
| DayOfWeek | `DW{4` | `Thu` |
| Duration | `{1,2,3,4` or `1D2H3M4S` | `1d 2h 3m 4s` |

A Duration is days, hours, minutes and seconds; a DayOfWeek is numbered
from Monday (1) to Sunday (7); a TimeZone is hours and minutes in steps of
15 minutes, both with the same sign (`{5,30` is UTC+05:30).

## Typing DATE objects

1. `2ND {` (on `(`) starts a record.
2. Type the fields, separated with `,`. Inside a record the `,` key
   separates fields instead of starting an exponent. `(−)` makes the field
   you're typing negative.
3. Press `ENTER`, or any operation or menu key. The closing brace (`2ND }`)
   is optional.

So the date March 14, 2024 is `2ND { 2024 , 3 , 14 ENTER`, and X shows
`D{2024,3,14}`.

The kind of object comes from the number of fields: 2 is a TimeZone, 3 a
Date, 4 a Duration, 6 a DateTime and 8 a ZonedDateTime. A Time and a
DayOfWeek need their name in front: type it with `ALPHA` (`ALPHA T` for
Time, `ALPHA D ALPHA W` for DayOfWeek) before `2ND {`, or use the first key
of the kind's submenu (**T{}**, **DW{}**, and so on), which starts the
record for you.

A Duration can also be typed compactly: digits followed by `ALPHA D`, `H`,
`M` or `S`. For example `1 ALPHA D 30 ALPHA M` is 1 day 30 minutes.

An impossible record, like February 30 or 23:59:60, shows "Err: Invalid"
and stays open so you can fix it with `DEL`. A record that can't be read
at all, such as one with 5 fields, shows "Err: Syntax".

Outside a record, `2ND }` is still DROP.

## How DATE objects are shown

`MODE`, row 4, chooses the style: **{..}** shows objects as you'd type them
(`D{2024,3,14}`, the default), and **".."** as readable text (`2024-03-14`,
`Thu`, `285d 8h 23m 59s`). Long values, such as a ZonedDateTime in ".."
mode, scroll with ◂ and ▸. SHOW (`2ND ENTRY`) always shows the typed
form.

![DATE objects in ".." mode](../../assets/guide/date-strings.png)

## Arithmetic with dates

`+` and `−` work on DATE objects, counting in the natural unit for each
kind:

| Kind | Numbers count in | Example |
| --- | --- | --- |
| Date, DayOfWeek | days | `D{2024,3,14}` `20 +` → `D{2024,4,3}` |
| Time, DateTime, ZonedDateTime | seconds | `T{15,36,1}` `3600 +` → `T{16,36,1}` |
| TimeZone | hours, in steps of 0.25 | `TZ{-8,0}` `1 +` → `TZ{-7,0}` |
| Duration | seconds | |

- **Subtracting two of the same kind** gives the difference: two Dates
  give days, two Times, DateTimes or ZonedDateTimes give seconds, two
  TimeZones give hours, and two Durations give a Duration.
- **A Duration** can be added to or subtracted from any kind in place of a
  number: `T{12,58,32}` plus `DR{0,2,33,0}` is `T{15,31,32}`.
- A number minus a DATE object is "Err: Data Type", except for a Duration.
- Times wrap around midnight; DateTimes and ZonedDateTimes move to the
  next or previous day as needed.

Four keys have DATE meanings when X holds a DATE object:

| Key | DATE function | Effect |
| --- | --- | --- |
| `×` | **CVTZ** | converts a Date, DateTime or ZonedDateTime to the time zone in the other level (a TimeZone, or hours) |
| `x⁻¹` | **DCUT** | splits a DateTime into a Date and a Time, or a ZonedDateTime into a DateTime and a TimeZone |
| `√` | **DSHK** | shrinks: a DateTime to its Date, a ZonedDateTime to its DateTime |
| `x²` | **DEXD** | extends: a Date to a DateTime at midnight, a DateTime to a ZonedDateTime in UTC |

Other math on a DATE object gives "Err: Data Type".

## Examples

| Keys | Result |
| --- | --- |
| `2ND { 2024,12,25 ENTER 2ND { 2024,3,14 −` | 286 (days from March 14 to Christmas 2024) |
| `2ND { 2024,3,14 ENTER 100 +` | `D{2024,6,22}` |
| `2ND { 2024,3,14`, then **D** > **DOW** | `DW{4}` (a Thursday) |
| `2ND { 2024,3,14,15,36,1,7 (−) ,0 ENTER 13 ×` | `DZ{2024,3,15,11,36,1,13,0}`: the same moment in Auckland (UTC+13) |
| then `x⁻¹` (DCUT) | X is the TimeZone `TZ{13,0}`, Y the DateTime |
| then **DT** > **DLNK** | joined back into the ZonedDateTime |
| `2ND { 2024,12,25,0,0,0 ENTER 2ND { 2024,3,14,15,36,1 −` | 24,654,239 (seconds) |
| then **DR** > **SEC→** | `DR{285,8,23,59}`: 285 days 8 h 23 min 59 s |

## The DATE submenus

| Menu | Holds |
| --- | --- |
| **D** | Dates: **D{}**, epoch days (**→ED**, **ED→**), epoch seconds (**→ES**, **ES→**), **LEAP** (is it a leap year? 1 or 0; also takes a plain year), **DOW**, **CVTZ**, the parts **.YR**, **.MON**, **.DAY**, and the casting row **DSHK**, **DEXD**, **DCUT**, **DLNK** |
| **T** | Times: **T{}**, **→S** and **S→** (seconds after midnight), **.HR**, **.MIN**, **.SEC** |
| **DT** | DateTimes: as for D, plus **.D{}** and **.T{}** to take out the date or the time |
| **TZ** | TimeZones: **TZ{}**, **→HR** and **HR→** (to and from hours), **.HR**, **.MIN** |
| **DZ** | ZonedDateTimes: as for DT, plus **ES→@** (epoch seconds to the application time zone), **.DT{** and **.TZ{** |
| **DR** | Durations: **DR{}**; **DAY→**, **HR→**, **MIN→**, **SEC→** make a Duration from a number; **→S** gives its seconds; **.DAY**, **.HR**, **.MIN**, **.SEC** |
| **DW** | DayOfWeek: **DW{}**; **→ISO** and **ISO→** (Monday 1 to Sunday 7), **→UNX** and **UNX→** (Sunday 0 to Saturday 6) |
| **EPCH** | the epoch for epoch days and seconds |
| **CLK** | the CE's clock and time zones |

**DLNK** joins a Date and a Time into a DateTime, or a DateTime and a
TimeZone into a ZonedDateTime, in either order in X and Y.

## Epochs

Epoch days and epoch seconds count from a starting date, the **epoch**.
**EPCH** chooses it:

| Item | Epoch |
| --- | --- |
| **UNIX** | 1970-01-01 (the default) |
| **NTP** | 1900-01-01 |
| **GPS** | 1980-01-06 |
| **TIOS** | 1997-01-01 |
| **Y2K** | 2000-01-01 |
| **CEPC** | a custom date, set with **EPC** from a Date in X (and shown with **EPC?**; 2050-01-01 by default) |

For example, `2ND { 2024,3,14,15,36,1,7 (−) ,0`, then **DZ** > **→ES**,
gives 1,710,455,761 with the UNIX epoch, and 3,919,444,561 with NTP.

## The clock and time zones

The CE has a real-time clock (the same one TI-OS sets in MODE). RPNCE uses
two time zones:

- the **clock time zone** (**CTZ**): the zone the CE's clock is set to;
- the **application time zone** (**ATZ**): the zone you want results in.

Both start at UTC. Set each from a TimeZone or from hours in X: if the
clock shows local time in Vancouver in summer, `7 (−)` **CTZ** and
`7 (−)` **ATZ**. **CTZ?** and **ATZ?** show them.

| Item | Gives |
| --- | --- |
| **NOW** | the current time as epoch seconds |
| **NOWD** | today's Date, in the application time zone |
| **NOWT** | the current Time, in the application time zone |
| **NWDZ** | the current ZonedDateTime, in the application time zone |
| **NWUT** | the current ZonedDateTime in UTC |
| **SETC** | sets the CE's clock from a ZonedDateTime (or a DateTime, taken as clock time) |

The date and time in the status bar come from the same clock, shown in the
clock's own time zone.


## Procedure: enter a date and count days

**Setup:** DEC; **MODE** row 4 **".."** for readable DATE output. A prior
DATE or time-zone setting is not required for this date-only example.

| Keys | X | Explanation |
| --- | --- | --- |
| `2ND { 2024 , 3 , 14 ENTER` | `2024-03-14` | Three fields identify a Date. |
| `100 +` | `2024-06-22` | A plain number adds calendar days to a Date. |
| **DATE** → **D** → **DOW** | `Sat` | Extract the day of the week. |

The braces are the second functions of the parenthesis keys. The comma
inside the record separates fields; it does not begin an exponent. A
DATE object occupies one stack level, even when it contains several fields.

## Splitting and joining objects

| Input X | Function | Result |
| --- | --- | --- |
| DateTime | **DSHK** | Date in X; time discarded. |
| ZonedDateTime | **DSHK** | DateTime in X; zone discarded. |
| Date | **DEXD** | DateTime at midnight. |
| DateTime | **DEXD** | ZonedDateTime with UTC offset zero. |
| DateTime | **DCUT** | Date in Y and Time in X. |
| ZonedDateTime | **DCUT** | DateTime in Y and TimeZone in X. |
| Date and Time in either operand order | **DLNK** | One DateTime in X; stack drops. |
| DateTime and TimeZone in either order | **DLNK** | One ZonedDateTime in X; stack drops. |

Only the input kinds listed above support these casts. Some other DATE
submenus contain the same cast labels for consistency, but selecting one
with an unsupported kind gives `Err: Data Type`.

DLNK attaches fields; CVTZ converts an instant to a different offset.
Those are different operations. For a ZonedDateTime, CVTZ preserves the
instant and changes its displayed local date and time. A Date or unzoned
DateTime is treated as UTC when passed to CVTZ.

## Epoch conversion details

Epoch conversions require a whole-number count. **→ED** returns whole
epoch days, discarding the time component; **→ES** returns epoch seconds.
For a ZonedDateTime, the offset is included when finding the UTC count.
The menu used for the reverse conversion determines the resulting kind:
**D**, **DT**, or **DZ**. In DZ, **ES→** produces UTC; **ES→@** produces
the application time zone.

Time zones are fixed offsets, not named regions. RPNCE does not infer
daylight-saving rules: update ATZ or CTZ when the intended offset changes.
A Duration counts elapsed seconds or days; it does not represent a calendar
month with a variable number of days.

## Clock operations that change settings

ATZ changes how application clock results are presented. CTZ tells RPNCE
what offset the hardware clock's existing reading represents. Neither
setting adjusts the hardware clock by itself. SETC writes a new hardware
clock reading; use it deliberately, because that clock is also used by
TI-OS. The status bar displays the hardware clock's local reading.

---

[Manual contents](../USER_GUIDE.md) · [Previous: 14. UNIT: unit conversions](14-units.md) · [Next: 16. Messages and errors](16-troubleshooting.md)
