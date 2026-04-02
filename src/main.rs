#![cfg_attr(target_os = "horizon", no_std)]
#![cfg_attr(target_os = "horizon", no_main)]
#![warn(clippy::disallowed_methods)]

#[macro_use]
extern crate alloc;

mod config;
mod ipc;
mod poll;
mod safe_writer;
mod snapshot;

use config::NxConfig;
use ipc::{PowerService, SaveFsService, TitleService};
use poll::{PollAction, Poller};

fn run<T: TitleService, P: PowerService, S: SaveFsService, Sleep: Fn(core::time::Duration)>(
    config: NxConfig,
    title_svc: T,
    power_svc: P,
    save_svc: S,
    sleep: Sleep,
) {
    let mut poller = Poller::new(config, title_svc, power_svc, save_svc);

    loop {
        match poller.tick() {
            PollAction::Sleep(duration) => {
                sleep(duration);
            }
            PollAction::TitleChanged { .. } => {}
            PollAction::SavesChanged {
                title_id: _,
                hex_id,
                changed_files,
            } => {
                let snap_dir = snapshot::snapshot_dir(poller.config(), &hex_id);
                snapshot::snapshot_changed_files(poller.save_fs(), &snap_dir, &changed_files);
            }
        }
    }
}

#[cfg(target_os = "horizon")]
extern crate nx;

#[cfg(target_os = "horizon")]
#[panic_handler]
fn panic_handler(_info: &core::panic::PanicInfo) -> ! {
    loop {
        core::hint::spin_loop();
    }
}

#[cfg(target_os = "horizon")]
fn main() {
    use ipc::nx_power::NxPowerService;
    use ipc::nx_savefs::NxSaveFsService;
    use ipc::nx_title::NxTitleService;

    let config = NxConfig::load();

    let title_svc = NxTitleService::new().expect("failed to connect to pm:dmnt/pm:info");
    let power_svc = NxPowerService::new().expect("failed to connect to psm");
    let save_svc = NxSaveFsService::new().expect("failed to connect to fsp-srv");

    run(config, title_svc, power_svc, save_svc, |d| {
        // nx::thread::sleep(d) — will use nx crate's sleep
        core::hint::spin_loop(); // placeholder
    });
}

#[cfg(not(target_os = "horizon"))]
fn main() {
    // Desktop stub — tests use #[cfg(test)] which provides std
    panic!("savetracker-nx requires the Nintendo Switch runtime");
}
