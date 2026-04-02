use alloc::collections::BTreeMap;
use alloc::string::String;
use alloc::vec::Vec;
use alloc::format;
use core::time::Duration;

use crate::config::NxConfig;
use crate::ipc::{PowerService, SaveFileMeta, SaveFsService, TitleService};

#[derive(Debug)]
pub struct TitleState {
    pub title_id: u64,
    pub hex_id: String,
    pub name: String,
}

#[derive(Debug)]
pub enum PollAction {
    Sleep(Duration),
    TitleChanged {
        old: Option<u64>,
        new: Option<u64>,
    },
    SavesChanged {
        title_id: u64,
        hex_id: String,
        changed_files: Vec<String>,
    },
}

pub struct Poller<T, P, S> {
    config: NxConfig,
    title_svc: T,
    power_svc: P,
    save_svc: S,
    current_title: Option<TitleState>,
    file_state: BTreeMap<String, SaveFileMeta>,
    is_paused: bool,
    tick_count: u64,
    title_check_counter: u64,
    power_check_counter: u64,
}

impl<T: TitleService, P: PowerService, S: SaveFsService> Poller<T, P, S> {
    pub fn new(config: NxConfig, title_svc: T, power_svc: P, save_svc: S) -> Self {
        Self {
            config,
            title_svc,
            power_svc,
            save_svc,
            current_title: None,
            file_state: BTreeMap::new(),
            is_paused: false,
            tick_count: 0,
            title_check_counter: 0,
            power_check_counter: 0,
        }
    }

    pub fn config(&self) -> &NxConfig {
        &self.config
    }

    pub fn save_fs(&mut self) -> &mut S {
        &mut self.save_svc
    }

    pub fn tick(&mut self) -> PollAction {
        self.tick_count += 1;

        self.poll_power();

        if let Some(action) = self.poll_title() {
            return action;
        }

        if let Some(action) = self.poll_saves() {
            return action;
        }

        let sleep_secs = if self.current_title.is_none() {
            self.config.idle_on_no_title.min(self.config.poll_title_interval)
        } else if self.is_paused {
            self.config.poll_title_interval
        } else {
            self.config.poll_save_interval
        };

        PollAction::Sleep(Duration::from_secs(sleep_secs))
    }

    fn ticks_for_interval(&self, interval_secs: u64) -> u64 {
        if self.config.poll_save_interval == 0 {
            return 1;
        }
        (interval_secs / self.config.poll_save_interval).max(1)
    }

    fn poll_power(&mut self) {
        self.power_check_counter += 1;
        let interval = self.ticks_for_interval(self.config.poll_power_interval);
        if self.power_check_counter < interval {
            return;
        }
        self.power_check_counter = 0;

        if self.config.pause_on_battery {
            self.is_paused = !self.power_svc.is_charging().unwrap_or(true);
        }
    }

    fn poll_title(&mut self) -> Option<PollAction> {
        self.title_check_counter += 1;
        let interval = self.ticks_for_interval(self.config.poll_title_interval);
        if self.title_check_counter < interval {
            return None;
        }
        self.title_check_counter = 0;

        let running = self.title_svc.get_running_application().ok()?;
        let old_id = self.current_title.as_ref().map(|t| t.title_id);

        match (old_id, running) {
            (Some(old), Some((_, new_id))) if old == new_id => None,
            (old, Some((_, new_id))) => self.switch_title(old, new_id),
            (Some(old), None) => {
                self.clear_title();
                Some(PollAction::TitleChanged {
                    old: Some(old),
                    new: None,
                })
            }
            (None, None) => None,
        }
    }

    fn switch_title(&mut self, old: Option<u64>, new_id: u64) -> Option<PollAction> {
        let hex_id = format!("{new_id:016X}");

        if self.config.is_excluded(&hex_id) {
            self.clear_title();
            return Some(PollAction::TitleChanged { old, new: None });
        }

        let name = self.config.title_name(&hex_id);
        self.current_title = Some(TitleState {
            title_id: new_id,
            hex_id,
            name,
        });
        self.file_state.clear();

        let _ = self.save_svc.mount_save(new_id);
        self.record_file_state();

        Some(PollAction::TitleChanged {
            old,
            new: Some(new_id),
        })
    }

    fn clear_title(&mut self) {
        self.save_svc.unmount_save();
        self.current_title = None;
        self.file_state.clear();
    }

    fn record_file_state(&mut self) {
        if let Ok(files) = self.save_svc.list_files() {
            for file in files {
                self.file_state.insert(file.path.clone(), file);
            }
        }
    }

    fn poll_saves(&mut self) -> Option<PollAction> {
        if self.current_title.is_none() || self.is_paused {
            return None;
        }

        let files = self.save_svc.list_files().ok()?;
        let changed = self.find_changed_files(&files);
        self.update_file_state(files);

        if changed.is_empty() {
            return None;
        }

        let title = self.current_title.as_ref()?;
        Some(PollAction::SavesChanged {
            title_id: title.title_id,
            hex_id: title.hex_id.clone(),
            changed_files: changed,
        })
    }

    fn find_changed_files(&self, files: &[SaveFileMeta]) -> Vec<String> {
        files
            .iter()
            .filter(|file| match self.file_state.get(&file.path) {
                Some(prev) => prev.size != file.size || prev.modified != file.modified,
                None => true,
            })
            .map(|file| file.path.clone())
            .collect()
    }

    fn update_file_state(&mut self, files: Vec<SaveFileMeta>) {
        self.file_state.clear();
        for file in files {
            self.file_state.insert(file.path.clone(), file);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ipc::mock::*;

    fn test_config() -> NxConfig {
        NxConfig::new()
            .with_poll_save_interval(0)
            .with_poll_title_interval(0)
            .with_poll_power_interval(0)
            .with_idle_on_no_title(1)
    }

    #[test]
    fn no_title_returns_sleep() {
        let mut poller = Poller::new(
            test_config(),
            MockTitleService { running: None },
            MockPowerService { charging: true },
            MockSaveFsService::new(),
        );

        match poller.tick() {
            PollAction::TitleChanged { old: None, new: None } => {}
            PollAction::Sleep(_) => {}
            other => panic!("expected Sleep or no-change TitleChanged, got {other:?}"),
        }
    }

    #[test]
    fn detects_new_title() {
        let mut poller = Poller::new(
            test_config(),
            MockTitleService {
                running: Some((1, 0x1234)),
            },
            MockPowerService { charging: true },
            MockSaveFsService::new(),
        );

        match poller.tick() {
            PollAction::TitleChanged {
                old: None,
                new: Some(0x1234),
            } => {}
            other => panic!("expected TitleChanged, got {other:?}"),
        }
    }

    #[test]
    fn detects_save_changes() {
        let dir = tempfile::tempdir().unwrap();
        std::fs::write(dir.path().join("save.dat"), b"v1").unwrap();

        let mut save_svc = MockSaveFsService::new();
        save_svc.add_title_saves(0x1234, dir.path().to_path_buf());

        let mut poller = Poller::new(
            test_config(),
            MockTitleService {
                running: Some((1, 0x1234)),
            },
            MockPowerService { charging: true },
            save_svc,
        );

        poller.tick();

        std::fs::write(dir.path().join("save.dat"), b"v2 longer").unwrap();

        match poller.tick() {
            PollAction::SavesChanged {
                title_id: 0x1234,
                hex_id: _,
                changed_files,
            } => {
                assert!(changed_files.contains(&String::from("save.dat")));
            }
            other => panic!("expected SavesChanged, got {other:?}"),
        }
    }

    #[test]
    fn paused_on_battery_skips_saves() {
        let dir = tempfile::tempdir().unwrap();
        std::fs::write(dir.path().join("save.dat"), b"v1").unwrap();

        let mut save_svc = MockSaveFsService::new();
        save_svc.add_title_saves(0x1234, dir.path().to_path_buf());

        let config = NxConfig::new()
            .with_poll_save_interval(0)
            .with_poll_title_interval(0)
            .with_poll_power_interval(0)
            .with_pause_on_battery(true);

        let mut poller = Poller::new(
            config,
            MockTitleService {
                running: Some((1, 0x1234)),
            },
            MockPowerService { charging: false },
            save_svc,
        );

        poller.tick();

        std::fs::write(dir.path().join("save.dat"), b"v2 longer").unwrap();

        match poller.tick() {
            PollAction::Sleep(_) => {}
            other => panic!("expected Sleep (paused on battery), got {other:?}"),
        }
    }

    #[test]
    fn excluded_title_ignored() {
        let config = NxConfig::new()
            .with_poll_title_interval(0)
            .with_exclude_titles(alloc::vec![String::from("0000000000001234")]);

        let mut poller = Poller::new(
            config,
            MockTitleService {
                running: Some((1, 0x1234)),
            },
            MockPowerService { charging: true },
            MockSaveFsService::new(),
        );

        match poller.tick() {
            PollAction::TitleChanged {
                old: None,
                new: None,
            } => {}
            other => panic!("expected excluded title to show as None, got {other:?}"),
        }
    }
}
