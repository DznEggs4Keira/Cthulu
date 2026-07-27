# The Unreal C++ Primer

What all the strange-looking macros actually do. Unreal C++ is regular C++ plus a code
generator; almost everything confusing comes from that code generator.

## The big idea: reflection

Plain C++ throws away type information when it compiles. The compiled program has no idea a
class was ever called `UInventoryWidget` or that it had a field named `SlotCount`.

Unreal needs that information at runtime — to show properties in the Details panel, to save
them to disk, to let a Blueprint call a function, to garbage-collect objects safely.

So before the real compiler runs, **Unreal Header Tool (UHT)** scans your headers, finds the
macros, and writes a `.generated.h` / `.gen.cpp` pair describing your types. That generated
code is what makes the editor able to see your class at all.

This explains several rules that otherwise look arbitrary:

- `#include "MyClass.generated.h"` **must be the last include.** UHT assumes it.
- Every `UCLASS` needs `GENERATED_BODY()` on its first line — that's where the generated code
  gets pasted in.
- Adding a `UPROPERTY` requires a real compile, not just Live Coding.
- Generated files live in `Intermediate/`. Never edit them; they're rewritten every build.

## The prefixes

Unreal requires a one-letter prefix on every type, and it is load-bearing — UHT enforces it.

| Prefix | Means | Example |
| --- | --- | --- |
| `U` | A `UObject` — engine-managed, garbage-collected, but *not* placeable in a level | `UInventoryWidget` |
| `A` | An `AActor` — a `UObject` that can be placed in a level and has a position | `AFishTank` |
| `F` | A plain struct or class with no engine management. Ordinary C++ | `FVector`, `FString` |
| `E` | An enum | `EFishSpecies` |
| `T` | A template container | `TArray`, `TMap` |
| `I` | An interface | `IInteractable` |

Choosing between `U` and `A`: *does this thing exist somewhere in the world?* A fish tank
sitting in the level is an `AActor`. An inventory widget, a save-game object, a subsystem —
those are `UObject`s.

## The four macros

### `UCLASS(...)`

Registers the class with the engine. Common specifiers:

- `Blueprintable` — a Blueprint can inherit from this
- `BlueprintType` — this type can be a variable in a Blueprint graph
- `Abstract` — cannot be spawned directly; must be subclassed
- `meta = (DisplayName = "...")` — friendlier name in the editor's class picker

### `UPROPERTY(...)`

Registers a variable. Does three jobs, and the third is the one that bites:

1. Shows it in the Details panel (`EditAnywhere` and friends)
2. Exposes it to Blueprints (`BlueprintReadOnly` / `BlueprintReadWrite`)
3. **Tells the garbage collector this pointer is in use**

That third one matters enormously:

```cpp
UPROPERTY()
TObjectPtr<UInventoryWidget> Widget;   // safe — GC knows about it

UInventoryWidget* Widget;              // DANGER — GC may delete it, leaving a dangling pointer
```

An unmarked `UObject*` member is a crash waiting to happen. **Any pointer to a `UObject`
stored as a member variable gets a `UPROPERTY()`** — even an empty one with no specifiers.
That empty macro is doing real work.

`TObjectPtr<T>` is the modern spelling of `T*` for members. Behaves like a raw pointer; use
it for `UPROPERTY` members and plain `T*` for local variables and parameters.

### `UFUNCTION(...)`

Registers a function so Blueprints (or the network layer) can reach it. See
[architecture.md](architecture.md#3-give-blueprints-hooks-to-react-to) for
`BlueprintCallable` vs `BlueprintImplementableEvent` vs `BlueprintNativeEvent`.

`BlueprintPure` is worth knowing: a `BlueprintCallable` that has no side effects and just
returns a value. It appears in the graph without an execution pin — a getter.

### `GENERATED_BODY()`

The paste-point for generated code. First line inside the class, no semicolon. If you get a
wall of incomprehensible errors from a class you just wrote, check this is present and that
`.generated.h` is the last include.

## Types you'll use constantly

| Unreal | Standard C++ equivalent | Note |
| --- | --- | --- |
| `FString` | `std::string` | Mutable text. `TEXT("hello")` wraps literals |
| `FName` | — | Immutable, case-insensitive, super fast to compare. For identifiers |
| `FText` | — | User-facing text that can be translated. Use for anything on screen |
| `TArray<T>` | `std::vector<T>` | The default list |
| `TMap<K,V>` | `std::unordered_map` | Key-value |
| `int32` / `float` | `int` / `float` | Unreal prefers explicit widths |

Always wrap string literals in `TEXT(...)`. It handles wide-character encoding; without it
you get conversion warnings or mangled text.

## Where the memory rules differ from normal C++

You almost never write `new` or `delete`.

- **`UObject`s** are created with `NewObject<UMyThing>(Outer)` and destroyed by the garbage
  collector when nothing holds a `UPROPERTY` reference to them.
- **Actors** are created with `World->SpawnActor<AMyActor>(...)` and removed with `Destroy()`.
- **Plain `F` structs** follow ordinary C++ rules — put them on the stack, pass by const
  reference.

The GC runs on a timer and collects anything unreachable from a root. "Reachable" means
"referenced through a `UPROPERTY`." Hence the rule above.

## Modules and `Build.cs`

`Source/Cthulu/Cthulu.Build.cs` lists which engine modules we link against:

```csharp
PublicDependencyModuleNames.AddRange(new string[] {
    "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
```

When you include an engine header and get `unresolved external symbol` at **link** time (not
compile time), the module isn't listed here. Add it and rebuild. Common additions:

| Want to use | Add |
| --- | --- |
| UMG widgets in C++ | `UMG` |
| Slate types directly | `Slate`, `SlateCore` |
| `AFunctionalTest` | `FunctionalTesting` |
| Gameplay Tags | `GameplayTags` |

Note `UMG` is currently only in `Cthulu.uproject`'s `AdditionalDependencies`, which affects
Blueprint tooling — not C++ linking. Writing C++ that touches `UUserWidget` beyond what
`Engine` provides means adding `"UMG"` to `Build.cs` too.

## Build vs. Live Coding

| Change | What to do |
| --- | --- |
| Edited a function body | Live Coding: `Ctrl+Alt+F11` in the editor. Seconds |
| Added/removed a `UPROPERTY`, `UFUNCTION`, or class | Close the editor, run a full build |
| Added a new `.h`/`.cpp` file | Close the editor, run a full build |
| Changed `Build.cs` or the `.uproject` | Full build; regenerate project files too |

Pushing Live Coding past what it supports gives strange errors or a crash on the next editor
launch. When something is inexplicable, close the editor and do a clean build before
debugging further.

## When you're stuck

- **`unresolved external symbol`** → a missing module in `Build.cs`, or a declared-but-not-
  defined function.
- **Errors pointing at `.generated.h`** → the include isn't last, or `GENERATED_BODY()` is
  missing, or UHT choked on a macro above.
- **Editor crashes on load after a code change** → delete `Binaries/` and `Intermediate/` and
  rebuild. This fixes a genuinely surprising number of things and costs only build time.
- **Blueprint says a C++ parent is missing** → the module failed to compile, so the class
  doesn't exist. Fix the build error first; don't touch the Blueprint.
