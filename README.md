# savetracker-nx

Nintendo Switch sysmodule for **reverse engineering game save files**. Runs
in the background while you play, automatically capturing binary snapshots
and diffs of save data changes to the SD card. Pair with the desktop
[savetracker](https://github.com/savetracker/savetracker) tool to analyze
snapshots, decode formats, and identify memory offsets.

**This is not a save backup tool.** If you want to back up or restore your
game saves, use [JKSV](https://github.com/J-D-K/JKSV) instead.

## How it works

1. Detects the running title via `pm:dmnt`
2. Mounts the title's save data partition (read-only) via `fsp-srv`
3. Polls for changes every 15 seconds (configurable)
4. On change, waits a configurable delay for the game to finish writing,
   then streams the save to `sdmc:/switch/savetracker/<title>/` as a
   `.snapshot` (full copy) or `.patch` (binary diff)
5. All I/O is streamed in 64KB chunks — works with saves of any size

## Layout

- `src/` — Switch-only sysmodule entry point and libnx glue.
- `lib/` — Pure-logic library (binary diff/patch, storage format). Host-testable.
- `tests/` — Unity-based unit tests, run under valgrind in CI.

## Build

### Switch sysmodule (NSP)

Requires devkitPro with `devkita64` and `libnx` installed.

```sh
make           # produces savetracker-nx.nsp and savetracker-nx.zip
```

The zip mirrors the SD card layout — extract to the SD root.

### Host tests

```sh
make -C tests test
make -C tests valgrind
```

## Install

Extract `savetracker-nx.zip` to the root of your Switch's SD card. After
rebooting, the sysmodule will autostart via `boot2.flag`. Tesla-Menu will
list it under the name "savetracker" via `toolbox.json`.

Copy `config/savetracker/savetracker.conf.example` to
`config/savetracker/savetracker.conf` and edit to customize timing,
LED signals, and battery behavior.

## License

BSD 2-Clause.
