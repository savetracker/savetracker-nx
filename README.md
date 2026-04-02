# savetracker-nx

> **This tool is designed for tracking save file changes over time for reverse
> engineering purposes. It is not a save backup tool.** Continuous polling may
> increase battery drain and reduce SD card lifespan due to frequent writes.
> Use at your own risk.

Nintendo Switch sysmodule that watches game saves and versions them to SD card.
Snapshots are analyzed later on PC with [savetracker](https://github.com/savetracker/savetracker).

## How it works

1. Sysmodule runs in the background while you play
2. Detects the active game via pm:dmnt/pm:info
3. Polls save files for changes every 5 seconds
4. Copies changed saves to SD card under `/switch/savetracker/<title_id>/`
5. Later, plug the SD card into your PC and run `savetracker analyze`

No network required during gameplay. Battery-conscious polling with configurable intervals.

## Requirements

- Nintendo Switch with [Atmosphere](https://github.com/Atmosphere-NX/Atmosphere) CFW
- Firmware 21.2.0 or below (22.0.0 not yet supported by Atmosphere)
- Saves exported to SD card via [Checkpoint](https://github.com/BernardoGiordano/Checkpoint)

## Install

Copy the sysmodule files to your SD card:

```
atmosphere/contents/42006D6B73740000/
  exefs.nsp          # the built sysmodule
  flags/
    boot2.flag       # empty file, tells Atmosphere to start at boot
```

## Configuration

Create `config/savetracker/config.toml` on the SD card root:

```toml
# Where to store snapshots
snapshot_base = "sdmc:/switch/savetracker"

# Maximum snapshots per save file before oldest is pruned
max_snapshots = 50

# Polling intervals (seconds)
poll_save_interval = 5
poll_title_interval = 30
poll_power_interval = 180

# How long to sleep when no game is running
idle_on_no_title = 90

# Stop snapshotting when on battery power (default: off)
pause_on_battery = false

# Human-readable names for titles
[title_overrides]
"01007EF00011E000" = "Zelda BOTW"
"0100F2C0115B6000" = "Zelda TOTK"

# Titles to ignore
exclude_titles = []
```

All fields are optional. Missing fields use the defaults shown above.

## Building from source

Requires:
- Rust nightly (pinned to 2025-12-01)
- [devkitPro](https://devkitpro.org/) with devkitA64 (`aarch64-none-elf-gcc`)
- [cargo-nx](https://github.com/aarch64-switch-rs/cargo-nx)

```
cargo nx build --release
```

The NSP is output to `target/aarch64-nintendo-switch-freestanding/release/savetracker-nx.nsp`.

Desktop tests (no Switch hardware needed):

```
cargo test --target x86_64-pc-windows-msvc
```

## Architecture

- **savetracker-core** — `no_std` crate providing the Storage trait and binary patch format
- **savetracker-nx** — Switch sysmodule using the [nx](https://github.com/aarch64-switch-rs/nx) crate for IPC and filesystem access

The sysmodule polls three tiers:
- Power state (3 min) — check battery/charging
- Title change (30s) — detect active game
- Save changes (5s) — stat files, snapshot on change

## Limitations

- Reads saves from Checkpoint's SD card export path, not directly from save data partition
- Charging detection limited (psm GetChargerType not exposed by nx crate)
- Cannot test on firmware 22.0.0 until Atmosphere supports it

## License

BSD-2-Clause
