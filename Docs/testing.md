# Testing

## What testing even means in a game project

The instinct is "how do you unit-test a game? The whole thing is a running simulation."
Fair. The answer is that you don't test the game — you test the **rules underneath** it, and
you use a different tool for each layer:

| Layer | Tool | Example |
| --- | --- | --- |
| Pure logic, no world needed | **Automation test** | "A fish worth 10 sold at 1.5× multiplier gives 15." |
| Logic that needs a live world | **Automation test + world fixture** | "Spawning the inventory widget adds 8 slots." |
| Gameplay in an actual level | **Functional test** (an actor in a map) | "Player walks to the sell tank and the monitor updates." |
| Does it even boot | **Smoke test** | "Load MMain, don't crash." |

The first row is where most value lives and it's the cheapest to write. Aim most effort there:
keep rules in plain C++ functions that don't need a `UWorld`, and they become trivial to test.
**That is the real reason to keep logic in C++ rather than Blueprint** — Blueprint graphs are
very hard to test automatically; C++ functions are easy.

## Running the tests

**From the editor (the way you'll usually do it):**
*Tools → Session Frontend → Automation* tab. Tick `Cthulu` in the tree, press *Start Tests*.
Results appear live, and a failed check tells you the file and line.

**From the command line:**

```powershell
# Everything under the Cthulu group
& "F:\Unreal Editors\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "F:\Work\Projects\Cthulu\Cthulu.uproject" `
  -ExecCmds="Automation RunTests Cthulu;Quit" -unattended -nopause -nullrhi -nosplash

# A single test — just use a longer prefix of its dotted name
-ExecCmds="Automation RunTests Cthulu.UI.InventoryWidget.HonoursBlueprintBaseContract;Quit"

# List what tests exist without running them
-ExecCmds="Automation List;Quit"
```

`-nullrhi` means "don't open a render window" — much faster, and required if this ever runs
unattended. The process exit code is 0 on pass, non-zero on failure.

Test logs land in `Saved/Automation/Logs/`.

## Writing a test

Tests live in `Source/Cthulu/Private/Tests/`, one file per class under test, named
`<ClassName>Test.cpp`. Look at `InventoryWidgetTest.cpp` — it is commented line by line.

The skeleton:

```cpp
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS   // compiled out of Shipping builds entirely

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMyThingTest,                              // C++ class name for the test
    "Cthulu.Category.Thing.WhatItChecks",      // where it appears in the test tree
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMyThingTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("2 + 2 should be 4"), 2 + 2, 4);
    return true;   // "the test ran"; pass/fail is decided by the TestX calls
}

#endif
```

Two things that trip people up:

1. **`return true` does not mean "passed."** It means the test finished without bailing out.
   Pass/fail is recorded by each `TestEqual` / `TestTrue` / `TestNotNull` call. Returning
   `false` marks the test as *unable to run*, which is a different thing from a failure.
2. **The dotted name is the whole UI.** `Cthulu.UI.InventoryWidget.HonoursBlueprintBaseContract`
   creates the nesting `Cthulu → UI → InventoryWidget → the test`. Keep the first segment
   `Cthulu.` on everything so our tests are filterable away from the engine's ~5000.

### The assertion helpers

`TestTrue`, `TestFalse`, `TestEqual`, `TestNotEqual`, `TestNull`, `TestNotNull`,
`TestValid` (for `TSharedPtr` / `TWeakObjectPtr`).

Each takes a description first. Write it as the *expectation*, not the operation — the
description is what you'll read at 1am when it fails:

```cpp
TestEqual(TEXT("Selling a 10-value fish at 1.5x should pay 15"), Payout, 15);   // good
TestEqual(TEXT("payout check"), Payout, 15);                                    // useless
```

Each helper returns a `bool`. Use that to bail before a null dereference:

```cpp
if (!TestNotNull(TEXT("Widget should have been created"), Widget))
{
    return true;   // stop here; every check below would crash
}
```

### Expecting an error

If the code under test deliberately logs an error, tell the framework or the test fails for
seeing it in the log:

```cpp
AddExpectedError(TEXT("Invalid slot index"), EAutomationExpectedErrorFlags::Contains, 1);
```

## What's worth testing

**Do test:**
- Rules and maths — prices, timers, capacity, state transitions.
- Boundaries — zero, empty, one-past-the-end, negative.
- Architectural contracts you want frozen. `InventoryWidgetTest.cpp` asserts the class stays
  `Abstract`; if someone deletes that specifier the test explains why it mattered.
- Anything you've fixed once already. A bug that shipped twice should have had a test.

**Don't bother testing:**
- That Unreal works. No test for "`UPROPERTY` saved my value."
- Exact visuals, animation timing, particle counts — expensive, brittle, and your eyes are
  better at it.
- Blueprint graph internals. If logic needs testing, that's the signal to move it to C++.

## Rebuilding after adding a test file

New `.cpp` files need a compile before the editor sees them:

```powershell
& "F:\Unreal Editors\UE_5.7\Engine\Build\BatchFiles\Build.bat" CthuluEditor Win64 Development `
  -Project="F:\Work\Projects\Cthulu\Cthulu.uproject" -WaitMutex
```

Close the editor first, or use *Live Coding* (`Ctrl+Alt+F11`) for changes to existing files.
Live Coding cannot add brand-new classes or files reliably — for those, close and rebuild.
