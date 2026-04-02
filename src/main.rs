#![cfg_attr(target_os = "horizon", no_std)]
#![cfg_attr(target_os = "horizon", no_main)]
#![cfg_attr(not(target_os = "horizon"), allow(dead_code, unused_imports))]
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
#[macro_use]
extern crate nx;

#[cfg(target_os = "horizon")]
nx::rrt0_define_default_module_name!();

#[cfg(target_os = "horizon")]
nx::rrt0_initialize_heap!();

#[cfg(target_os = "horizon")]
#[panic_handler]
fn panic_handler(_info: &core::panic::PanicInfo) -> ! {
    loop {
        core::hint::spin_loop();
    }
}

#[cfg(target_os = "horizon")]
#[unsafe(no_mangle)]
fn main() {
    use ipc::nx_power::NxPowerService;
    use ipc::nx_savefs::NxSaveFsService;
    use ipc::nx_title::NxTitleService;

    let config = NxConfig::load();

    let title_svc = match NxTitleService::new() {
        Ok(s) => s,
        Err(_) => return,
    };
    let power_svc = match NxPowerService::new() {
        Ok(s) => s,
        Err(_) => return,
    };
    let save_svc = match NxSaveFsService::new() {
        Ok(s) => s,
        Err(_) => return,
    };

    run(config, title_svc, power_svc, save_svc, |d| {
        let nanos = d.as_nanos() as i64;
        let _ = nx::thread::sleep(nanos);
    });
}

#[cfg(not(target_os = "horizon"))]
fn main() {
    panic!("savetracker-nx requires the Nintendo Switch runtime");
}
