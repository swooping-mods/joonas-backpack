# J.O.O.N.A.S. Backpack: behavior and implementation

Describes the exported implementation **0.3.0**. The linked source and metadata define this version; gameplay qualification is a separate requirement.

## Behavior

A $5,000 wearable sweeper requires at least twelve owned upgrade levels and at least half the native total. Select its Equipment slot to deploy the textured backpack and rigid articulated arms. Two bounded terrain-only brush strokes call native CleanToolStroke, using a separately named 20 kg registered storage. A full payload reserves a real catcher and launches a native capsule carrying the same withdrawn mass. Vanilla owns flight and arrival/deposit.

## State and lifecycle

Purchased ownership uses the provider sidecar; quickslot IDs are saved natively. Shared mesh resources survive holstering; instance arms/visuals are destroyed on holster and shared resources release after instances on world teardown. Native storage/cleaning methods own cargo changes. Reachability, terrain contacts and brush-to-ground separation gate each stroke.

## Integration and configuration

Runtime dependencies: [shop-framework](https://github.com/swooping-mods/shop-framework), [equipment-framework](https://github.com/swooping-mods/equipment-framework), [hints-framework](https://github.com/swooping-mods/hints-framework).

The [module metadata](../source/mods/joonas-backpack/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

| Service | ABI | Table size |
|---|---:|---|
| Requires `shop-framework.shop` | 1 | at least 56 bytes |
| Requires `equipment-framework.equipment` | 1 | at least 32 bytes |
| Requires `hints-framework.hints` | 1 | at least 24 bytes |

Packaged defaults in [settings.ini](../source/mods/joonas-backpack/settings.ini); supported bounds and interpretation remain in the linked parser.

| Section | Setting | Default |
|---|---|---|
| `(root)` | `enabled` | `1` |
| `(root)` | `equipment_key` | `106` |
| `(root)` | `action_key` | `98` |
| `(root)` | `speed` | `38` |

## Implementation sources

- [mod.c](../source/mods/joonas-backpack/mod.c)
- [motion.h](../source/mods/joonas-backpack/motion.h)
- [tool_runtime.h](../source/mods/joonas-backpack/tool_runtime.h)
- [settings.h](../source/mods/joonas-backpack/settings.h)

## Verification boundary

Arm motion is kinematic articulation, not collision simulation. Visual fit, obstacles/slopes, cleaning/storage/catcher transactions, in-flight persistence and remote presentation require game checks; purchase tests alone do not qualify them.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
