# Cthulu — Project Documentation

Notes for **me** (the developer), written so that future-me can pick this project back up
without re-deriving everything. If something here is wrong, fix it — a stale doc is worse
than no doc.

## The docs

| Doc | What it answers |
| --- | --- |
| [architecture.md](architecture.md) | What systems exist, where they live, and how they connect |
| [testing.md](testing.md) | How testing works in Unreal, and how to run ours |
| [unreal-cpp-primer.md](unreal-cpp-primer.md) | What all the C++ / Unreal "magic" actually does |

## Where everything is

```
Cthulu/
├── Config/            Project settings as .ini files (renderer, input, default map)
├── Content/           All assets. Binary, Git-LFS tracked, only editable in the editor.
│   ├── Maps/          MMain.umap — the only level right now
│   ├── Materials/     Monitors/ — the CRT camera-feed materials + render targets
│   ├── Code/UI/       Widget Blueprints (children of our C++ widget classes)
│   └── Developers/    Personal scratch space. Nothing shipping goes here.
├── Docs/              You are here
└── Source/Cthulu/
    ├── Public/        Headers other files may include
    ├── Private/       Implementation, plus Tests/
    └── Cthulu.Build.cs  Which engine modules we link against
```

Everything under `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/` and
`Cthulu.sln` is generated. Deleting them is always safe; they come back on the next build.

## The one rule that shapes everything else

**Behaviour and rules live in C++. Look, feel, and tuning live in Blueprints.**

Every C++ gameplay class is written as an `Abstract` base. The real thing used in the game
is a Blueprint that inherits from it. That way the numbers, art, and layout can be changed
in the editor — with instant feedback and no recompile — while the logic stays in one
readable, testable place.

See [architecture.md](architecture.md#the-c-blueprint-split) for how to actually write a
class that way.
