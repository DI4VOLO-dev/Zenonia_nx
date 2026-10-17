# ZENONIA 1 & 2 for Nintendo Switch

> **AI-assisted project:** this port was written with the help of AI ([Claude](https://claude.ai), by Anthropic) and tested on real hardware by the maintainer.

Unofficial homebrew ports of Gamevil's **ZENONIA** and **ZENONIA 2** (32-bit Android versions) to the Nintendo Switch, built on [aks796's android32 runtime](https://github.com/aks796/android32). ZENONIA 3 is a work in progress and has no release yet.

**No game files are included or distributed here.** You must provide your own APKs.

## What you need

- A Switch running Atmosphère, with [sphaira](https://github.com/ITotalJustice/sphaira) installed.
- The exact APK for each game (the port refuses any other build):

| Game | Get the APK from | Package / version |
| --- | --- | --- |
| ZENONIA 1 | [r/Zenonia: "Zenonia 1 Remastered"](https://www.reddit.com/r/Zenonia/comments/1c9xiw6/zenonia_1_remastered/) | `com.gamevil.zenonia` **1.6.6** (versionCode 166) |
| ZENONIA 2 | [r/Zenonia: "Zenonia Remastered update"](https://www.reddit.com/r/Zenonia/comments/1cectmo/zenonia_remastered_update/) | `com.gamevil.zenonia2.global` **1.0.3** (versionCode 103) |

Full credit to [u/Ill-Hovercraft8548](https://www.reddit.com/user/Ill-Hovercraft8548/) for the Remastered APKs these ports are built and tested against.

APK source posts:
- ZENONIA 1: https://www.reddit.com/r/Zenonia/comments/1c9xiw6/zenonia_1_remastered/
- ZENONIA 2: https://www.reddit.com/r/Zenonia/comments/1cectmo/zenonia_remastered_update/

**Nothing needs to be extracted from the APK.** The launcher reads the `.apk` directly and unpacks the game's library by itself on the first launch.

## Install

1. Download `zenonia_nx.nro` (ZENONIA 1) and/or `zenonia2_nx.nro` (ZENONIA 2) from [Releases](../../releases).
2. Copy the NRO and the matching APK (any file name ending in `.apk`) to `sd:/switch/zenonia_nx/` or `sd:/switch/zenonia2_nx/`.
3. In sphaira: **Homebrew → the game → Install Forwarder**, then launch the new HOME icon. The first launch unpacks the library (once).

To remove a port, delete its forwarder and its `atmosphere/contents` folder (the ID is in `title_id.txt`, inside the game's folder).

## Controls

| Switch | ZENONIA 1 | ZENONIA 2 |
| --- | --- | --- |
| D-pad / left stick | Move | Move (diagonals too) |
| A | Confirm / attack | Confirm / attack |
| B | Game menu / cancel | Game menu |
| + | Game menu | Game menu |
| − | Tap-anywhere (skip text) | Back / cancel |
| X | Map | Map |
| Y | Swap quick slot / skip | Swap skill |
| L / R | Menu tabs | L: back, R: swap skill |
| ZL, ZR, L3, R3 | Quick slots 1-4 | Quick slots 1-4 |
| Touch screen | Skips text | Menus, NPCs |

Options are in `config.ini`, created in the game folder on the first launch (resolution, smoothing, volume, swap A/B).

## Problems

Logs are written to `debug.log` and `crash.log` in the game folder. Please attach them to an [issue](../../issues), with the game and what you were doing.

## Build from source

Needs Docker, [libnx32](https://github.com/aks796/libnx32) 4.12.0+ (next to the project, in `../libnx32/prefix`) and the prebuilt [mesa32](https://github.com/aks796/mesa32) (`lib/` and `include/` into `portlibs32/`).

```
git clone --recursive <this repo>      # ZENONIA 1: main, tag zenonia1-v1.0.3
git checkout zenonia2                  # ZENONIA 2: tag zenonia2-v1.0.2
./build.sh && launcher/build.sh        # gives launcher/<game>.nro
```

## Credits

- Port: DI4VOLO, with [Claude](https://claude.ai) (Anthropic) as AI co-author.
- APKs: [u/Ill-Hovercraft8548](https://www.reddit.com/user/Ill-Hovercraft8548/).
- Runtime, libnx32, mesa32: [aks796](https://github.com/aks796); libnx32 comes from [vita2hos](https://github.com/xerpi/vita2hos).
- Toolchain: [devkitPro](https://devkitpro.org).
- ZENONIA is © Gamevil. This project is not affiliated with or endorsed by Gamevil.

## License

MIT, for the code in this repository only (see `LICENSE`). The games themselves are not covered.
