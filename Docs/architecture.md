# Architecture

## The C++ / Blueprint split

The project is built in two layers.

**C++ layer — the rules.** How much a fish is worth. When a tank goes bad. What "buy" means.
Written once, tested, and hard to break by accident.

**Blueprint layer — the presentation and the tuning.** Which mesh, which colour, how many
seconds, where the button sits. Changed in the editor in seconds, no compile.

Concretely, a C++ class in this project should be written so a Blueprint can subclass it and
customise it *without touching C++ again*. That means three habits:

### 1. Mark base classes `Abstract`

```cpp
UCLASS(Abstract, Blueprintable, BlueprintType)
class CTHULU_API UInventoryWidget : public UUserWidget
```

- `Abstract` — this class can never be placed or spawned directly. It exists to be inherited
  from. If you try to drag it into a level, the editor stops you. This is a *good* error: it
  means you meant to use the Blueprint child.
- `Blueprintable` — a Blueprint is allowed to use it as a parent class.
- `BlueprintType` — a variable of this type can exist inside a Blueprint graph.

### 2. Expose tuning values, don't hardcode them

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory",
          meta = (ClampMin = "1", ToolTip = "How many item slots the bar shows."))
int32 SlotCount = 8;
```

The `= 8` is a *default*, not a decision. The Blueprint child can override it in the editor's
Details panel. `ClampMin` stops a nonsense value being typed in, and `ToolTip` explains it to
whoever is editing — including future-me.

Which specifier to use:

| Specifier | Means |
| --- | --- |
| `EditDefaultsOnly` | Edit on the Blueprint class (the "template"), same for every instance |
| `EditInstanceOnly` | Edit per placed actor in the level |
| `EditAnywhere` | Both |
| `BlueprintReadOnly` | Blueprint graphs can read it |
| `BlueprintReadWrite` | Blueprint graphs can read and change it |

Default to `EditDefaultsOnly, BlueprintReadOnly` and loosen only when there's a reason.
Read-only by default means the C++ stays in charge of the rule.

### 3. Give Blueprints hooks to react to

Three ways C++ and Blueprint talk to each other:

| Markup | Direction | Use it when |
| --- | --- | --- |
| `UFUNCTION(BlueprintCallable)` | BP calls C++ | The Blueprint wants to *do* something: "sell this fish" |
| `UFUNCTION(BlueprintImplementableEvent)` | C++ calls BP, no C++ body at all | Pure presentation: "play the sold animation" |
| `UFUNCTION(BlueprintNativeEvent)` | C++ calls BP, with a C++ default | There's sensible default behaviour a Blueprint may override |

`BlueprintImplementableEvent` is the workhorse for designer-facing hooks. C++ decides *when*
something happened; the Blueprint decides what it looks like:

```cpp
/** Fired after an item is added. Use this to play feedback — sound, animation, popup. */
UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
void OnItemAdded(int32 SlotIndex);
```

You declare it and never write a body — Unreal generates one. Calling `OnItemAdded(3)` from
C++ fires the event node in the Blueprint graph.

---

## Systems

### Inventory UI

- `Source/Cthulu/Public/InventoryWidget.h` — `UInventoryWidget`, an abstract `UUserWidget` base.
- Intended child: a widget Blueprint in `Content/Code/UI/`.

Currently a bare base class with no behaviour — the shape is set, the logic isn't written yet.
The previous `WInventoryBar` widget Blueprint was deleted in `20fa5b8` and needs rebuilding.

**Reparenting an existing widget Blueprint to a C++ class:** open the Blueprint → *File →
Reparent Blueprint* → pick the C++ class. Do this before adding much to the Blueprint; changing
the parent later can drop variables and break connections.

### CRT camera-feed monitors

The facility is watched through in-world CRT screens, each showing a live feed of a fish tank.

The chain, for each monitor:

```
SceneCaptureComponent2D in MMain   →   RT_Tank*.uasset   →   M_Tank*.uasset   →   monitor mesh
   (a camera that renders               (a Render Target:      (samples the RT and     (the screen
    to a texture instead of              a texture the         adds scanlines /         in the
    to the screen)                       engine writes to      digital static)          level)
                                         every frame)
```

Assets in `Content/Materials/Monitors/`:

| Render target | Material | Feed |
| --- | --- | --- |
| `RT_TankMain` | `M_TankMain` | Main tank |
| `RT_TankSell` | `M_TankSell` | Sell tank |
| `RT_TankExtra` | `M_TankExtra` | Extra tank |
| `RT_TankMate` | *(none yet)* | Mate tank — render target exists, material missing |

**To add a monitor:** create an `RT_` render target, create an `M_` material that samples it,
place a `SceneCaptureComponent2D` in `MMain` with its *Texture Target* set to the new render
target, and apply the material to the screen mesh.

**Cost warning:** every scene capture renders the world an extra time, every frame. Four
monitors ≈ five full renders per frame. If the framerate falls off a cliff, this is the first
place to look. The fix is to lower each render target's resolution, or set the capture to
update only when the monitor is actually visible.

---

## Project-wide settings worth knowing

In `Config/DefaultEngine.ini`, chosen deliberately — don't "fix" them without a reason:

- **Static lighting is off**; Lumen provides dynamic GI and reflections. Never bake lightmaps.
- **Substrate** materials are enabled — the newer, more flexible material system.
- **Ray tracing** and **virtual shadow maps** are on. DX12 / Shader Model 6 required.
- `MMain` is both the game default map and the editor startup map.
- `ActiveGameNameRedirects` maps the old `TP_Blank` template name to `/Script/Cthulu`. Leave
  those lines alone or older assets stop loading.
