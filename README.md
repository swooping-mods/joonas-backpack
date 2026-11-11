# J.O.O.N.A.S. Backpack

Buy the backpack in Modded Shop after its late-game upgrade requirement.

## Install

Install [Moon Sweepers Mod Loader](https://github.com/swooping-mods/moon-sweepers-mod-loader) first.

Download [joonas-backpack-0.3.0.msmod](content/joonas-backpack-0.3.0.msmod), then drag and drop the `.msmod` file onto the [Moon Sweepers Mod Loader](https://github.com/swooping-mods/moon-sweepers-mod-loader) window. You can also use **Add mods…** (older loader builds: **Install mod package**).

Select the mod and its required frameworks, click **Prepare files**, then **Launch**.

Required mods: [shop-framework](https://github.com/swooping-mods/shop-framework), [equipment-framework](https://github.com/swooping-mods/equipment-framework), [hints-framework](https://github.com/swooping-mods/hints-framework).

## Controls

- Buy the backpack in Modded Shop after its late-game upgrade requirement.
- Select its Equipment hotbar slot to deploy the textured RoboScrub backpack and rigid sweeping arms.
- Native shop/equipment integration implemented; gameplay testing deferred.

## Compatibility

Version 0.3.0. Targets the pinned Steam Windows x64 IL2CPP build of Moon Sweepers (Steam app 5335510). On Linux, the game runs through Proton. Experimental: back up saves before using save-affecting mods.

## Build

Run `python3 build.py --check` to verify source hashes, then `python3 build.py` to write a build under `.local/build/`. Requires installed Clang, lld-link and llvm-dlltool.

## Credits

See [CREDITS.txt](CREDITS.txt) and [third-party notices](vendored/THIRD_PARTY_NOTICES.md).
