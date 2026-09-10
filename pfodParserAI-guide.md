# pfodParser — AI Guide to Timers and Delays (`pfodDelay` / `millisDelay`)

Audience: an AI writing or reviewing Arduino/C++ sketch code against this
library, that needs a non-blocking "is it time yet?" timer — a poll
interval, a retry backoff, an LED flash, a debounce window — anything that
would otherwise be a hand-rolled `millis()` comparison.

**This is not the library's design/generate/complete guide.** For building
a pfod menu/dwg design, running the pfodWeb Designer, and completing the
generated `pfodMainMenu`/`Dwg_<Name>` code, read
[`pfodWeb/docs/pfodAI-guide.md`](pfodWeb/docs/pfodAI-guide.md) instead —
that document covers the whole three-stage workflow and its own rules
(never modify library code, ask before adding a dependency, build only what
was asked). This guide covers one narrow, frequently-needed piece that
document doesn't: **timing**. The same "never modify library code" rule
applies here too — see the last section.

Everything below is drawn from the actual sources in this repository:
`src/pfodDelay.h/.cpp` and the four worked sketches in
`examples/pfodDelay_Examples/`. The canonical write-up, with more worked
examples than either of those, is Forward Computing's own tutorial:
<https://www.forward.com.au/pfod/ArduinoProgramming/TimingDelaysInArduino.html>
— read it if anything here is ambiguous; this guide's API and idiom follow
it exactly.

---

## 1. Why not a plain `millis()` comparison

The obvious hand-rolled version looks like this, and it is a real, common
bug, not a style nitpick:

```cpp
unsigned long nextDueMs = 0;
...
void loop() {
  if (millis() < nextDueMs) {
    return; // not due yet
  }
  ... do the thing ...
  nextDueMs = millis() + INTERVAL_MS;
}
```

`millis()` **overflows** (wraps back to 0) after roughly 49.7 days of
continuous uptime. `nextDueMs` is a stored target that stays a large
pre-wrap value while `millis()` itself resets near 0 — so `millis() <
nextDueMs` reads "not due yet" for a long stretch after the wrap, silently
stalling whatever this timer drives for potentially weeks, with no crash
and no error to notice it by. It is exactly the kind of bug that never
shows up on the bench (nobody soak-tests for 49 days before shipping) and
only shows up on a device that has been running unattended for a long
time — which, for anything solar/battery/home-automation related, is
every device, always.

`pfodDelay`/`millisDelay` exist specifically to make this class of bug
impossible: internally they compare via **subtraction** —
`(millis() - startTime) >= mS_delay` — which is wraparound-safe by the
unsigned-arithmetic identity that makes `(a - b)` come out correct even
when `a` has wrapped past `b`. You never need to reason about the wrap
yourself; use the class and it's handled.

**Use `pfodDelay`/`millisDelay` for every recurring "is it due yet?"
timer.** The only comparisons that may stay as plain `millis()` are
one-time, first-`N`-seconds-of-uptime boot gates that latch permanently
true via their own flag before 49.7 days could ever be reached (e.g. "wait
20s after boot before the first NTP-dependent fetch") — those are
structurally unreachable to the wraparound bug, not exceptions to the
rule.

---

## 2. Two names, one class — which header to include

| Class | Header | Library | When to use it |
|---|---|---|---|
| `pfodDelay` | `<pfodDelay.h>` | **This library** (`src/pfodDelay.h/.cpp`) | Default choice in any sketch that already depends on `pfodParser` — no extra library to add. |
| `millisDelay` | `<millisDelay.h>` | Forward Computing's separate **SafeString** library | Only if the sketch already depends on SafeString for something else, or isn't using pfodParser at all. |

They are **the same class under two names** — identical public API, same
private fields (`pfodDelay` even keeps SafeString's field-naming style),
same semantics, both implement the same tutorial. `pfodDelay` is this
library's own bundled copy, provided so a pfodParser sketch gets a
wraparound-safe timer without pulling in SafeString as a second
dependency. **Do not add SafeString to a pfodParser sketch just to get
`millisDelay` — use `pfodDelay`, it is already there.** If you are ever
reading or writing code against SafeString directly, the exact same
guidance in this document applies to `millisDelay`; just substitute the
class name and header.

---

## 3. API reference

```cpp
#include <pfodDelay.h>
pfodDelay myTimer; // one instance per independent timer, usually a static/global
```

| Method | Does |
|---|---|
| `start(unsigned long delay)` | Arms the timer for `delay` ms from **now**. `delay == 0` is a documented special case: `justFinished()` returns `true` the very next time it's called — see §5's "fire immediately" idiom. |
| `stop()` | Disarms it. `justFinished()` will never return `true` again until `start()`, `restart()` or `repeat()` is called. |
| `repeat()` | Re-arms for the *same* delay, but from the timer's **previous target** (`startTime += mS_delay`), not from now. Use this to re-arm a regular repeating timer right after `justFinished()` returns `true` — see §5's drift note. |
| `restart()` | Re-arms for the same delay, starting from **now** (`start(delay())`). Use when the delay should reset relative to some fresh event, not the old schedule. |
| `finish()` | Forces the timer to end **early** — the *next* `justFinished()` call returns `true` regardless of elapsed time. |
| `justFinished()` | **The one you check every call.** Returns `true` exactly once, the first time it's called after the delay has elapsed (or after `finish()`). Calling it also stops the timer (`isRunning()` becomes `false` after a `true` return) — that's what makes `repeat()`/`restart()` necessary to keep a recurring timer going. |
| `isRunning()` | `true` if the timer is currently armed and will (eventually) make `justFinished()` return `true`. **Query only — see §4 for why this must never gate `justFinished()`.** |
| `getStartTime()` | The `millis()` value at the last `start()`/`repeat()`/`restart()`. `0` if never started. |
| `remaining()` | ms left until due; `0` if already finished or stopped. Doesn't mutate state — safe to call for a progress readout without disturbing `justFinished()`'s own bookkeeping. |
| `delay()` | The delay value last passed to `start()` (or carried forward by `repeat()`/`restart()`). |

---

## 4. The one rule that matters: how `justFinished()` must be called

> **`justFinished()` must be checked unconditionally, at the outermost
> level, on every single pass of `loop()` (or whatever task loop owns this
> timer) — never folded into a compound condition, and never nested inside
> other gating logic that could skip calling it on some passes.**

This is the single most common mistake, and it looks harmless because it
often still *works* in testing. The wrong shape:

```cpp
// WRONG — do not write this
if (myTimer.isRunning() && !myTimer.justFinished()) {
  return;
}
```

Why it's wrong, beyond "it's not the documented idiom": `justFinished()`
is **edge-triggered and stateful** — it returns `true` exactly once, and
that one call is also what stops the timer. Burying it inside a
short-circuiting `&&` means there exist code paths where `isRunning()` is
`false` and `justFinished()` is never evaluated at all on that pass — for
a one-shot timer that's usually harmless, but the pattern trains you to
write the same thing for cases where it isn't (a timer another branch
might stop, or one whose "just expired" edge you need to react to exactly
once). The tutorial's own wording: `justFinished()` "must be called every
loop and ... at the outer most level ... not inside another `if()` `while()`
`case()` etc statement. It can be inside a method call as long as that
method is called every loop."

The correct shapes, straight from the tutorial and from every example in
`examples/pfodDelay_Examples/`:

```cpp
// Reacting to expiry — outermost, unconditional:
if (myTimer.justFinished()) {
  // do the thing
}
```

```cpp
// Bailing out until due — same check, inverted, still outermost:
if (!myTimer.justFinished()) {
  return;
}
// do the thing
```

**Arming a timer is a separate, unconditional check of its own — never
combined with the `justFinished()` check above:**

```cpp
if (!myTimer.isRunning()) {
  myTimer.start(INTERVAL_MS); // (or start(0) -- see §5)
}
if (!myTimer.justFinished()) {
  return;
}
```

`isRunning()` is for questions like "should I print a different message
depending on whether this is still counting down" (see
`SingleDelay.ino`'s `F` branch, which checks `isRunning()` purely to
choose a log line — it never touches `justFinished()`'s own control flow)
— not for deciding whether to evaluate `justFinished()`.

---

## 5. Recipes

### 5.1 One-shot delay (do X, then Y once some time later)

```cpp
static pfodDelay ledDelay;

void onSomeEvent() {
  digitalWrite(LED_PIN, HIGH);
  ledDelay.start(10000); // 10s from now
}

void loop() {
  if (ledDelay.justFinished()) {
    digitalWrite(LED_PIN, LOW);
  }
}
```

Ground truth: `examples/pfodDelay_Examples/SingleDelay/SingleDelay.ino`.
Nothing arms this timer up front — it only starts in response to an
external trigger (there, a serial command). `justFinished()` is still
checked unconditionally every `loop()`, and simply never fires until
something has called `start()`.

### 5.2 Poll every N ms, first check fires immediately

The common shape for a background poll job (fetch a sensor, hit an API)
that should run once as soon as it's first ticked, then on a fixed
interval after that — no separate "has this ever run" flag needed:

```cpp
static pfodDelay pollTimer;

void checkPoll() {
  if (!pollTimer.isRunning()) {
    pollTimer.start(0); // 0 => justFinished() reads true on the very next check
  }
  if (!pollTimer.justFinished()) {
    return;
  }

  bool ok = doTheFetch();

  unsigned long nextDelay = ok ? POLL_INTERVAL_MS : RETRY_DELAY_MS;
  pollTimer.start(nextDelay); // re-arm for the next cycle
}
```

Why `start(0)` and not some other bootstrap trick: it's a documented
special case (`start()`'s own doc comment: `delay == 0 means
justFinished() returns true on first call`), so the very first call to
`checkPoll()` fires the fetch immediately, and every call after that
follows the real interval. 

### 5.3 Regular repeating timer, no drift (`repeat()`)

`RepeatingDelay.ino` flashes an LED on a fixed period. The key call is
`repeat()`, not `start()` again:

```cpp
static pfodDelay ledDelay;
static bool ledOn = false;

void startFlashing() {
  ledDelay.start(FLASH_TIME);
  ledOn = true;
  digitalWrite(LED_PIN, HIGH);
}

void loop() {
  if (ledDelay.justFinished()) {
    ledDelay.repeat(); // NOT start(FLASH_TIME) again -- see below
    ledOn = !ledOn;
    digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
  }
}
```

`repeat()` re-arms from the **previous target time**
(`startTime = startTime + mS_delay`), not from whatever `millis()` happens
to be when `repeat()` is actually called. If `loop()` gets delayed a few ms
by other work before it gets around to checking `justFinished()`, that
delay does not accumulate cycle over cycle — the schedule stays locked to
the original period. Calling `start(FLASH_TIME)` again instead would
re-arm from "now", and now silently drifts later by however late each
`loop()` pass happened to be, cycle after cycle. Use `repeat()` for
anything that should tick on a steady clock (a metronome, a fixed sample
rate); use `restart()` (or a fresh `start()`) when the delay is meant to
reset relative to a real external event instead (e.g. re-arming a
"user has been idle for N seconds" timer every time real activity is
seen — there is no "previous schedule" worth preserving).

### 5.4 Force an early finish (`finish()`)

```cpp
if (userCancelled) {
  ledDelay.finish(); // next justFinished() call returns true right away
}
```

`finish()` doesn't itself make `justFinished()` return `true` — it flags
that the *next* call should. `PrintDelay.ino` and `SingleDelay.ino`'s `F`
command both demonstrate this: calling `finish()` while a delay is not
even running is harmless (there's nothing to finish early), and calling it
while running lets the very next unconditional `justFinished()` check pick
it up exactly like a natural expiry — no separate "was it cancelled"
branch needed at the call site that reacts to it.

---

## 6. Never modify `pfodDelay.h`/`.cpp`

Same rule as every other file under `src/` in this library (see
`pfodWeb/docs/pfodAI-guide.md` §"Both: never modify the pfodParser
library"): `pfodDelay` is shared across every sketch on the machine, gets
overwritten by the next library update, and a sketch that depends on a
locally-patched copy is not portable. If the class is genuinely missing
something, use it as-is and build the missing behaviour in your own code
(a wrapper, an extra flag beside your `pfodDelay` instance), or report the
gap rather than patching the library file.
