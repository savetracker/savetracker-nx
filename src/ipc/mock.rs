// Mock implementations use std for filesystem access in tests.
// This module only compiles for desktop test builds, not the Switch target.

use alloc::collections::BTreeMap;
use alloc::string::String;
use alloc::vec::Vec;

use super::{IpcError, PowerService, SaveFileMeta, SaveFsService, TitleService};

pub struct MockTitleService {
    pub running: Option<(u64, u64)>,
}

impl TitleService for MockTitleService {
    fn get_running_application(&self) -> Result<Option<(u64, u64)>, IpcError> {
        Ok(self.running)
    }
}

pub struct MockPowerService {
    pub charging: bool,
}

impl PowerService for MockPowerService {
    fn is_charging(&self) -> Result<bool, IpcError> {
        Ok(self.charging)
    }
}

pub struct MockSaveFsService {
    mounted_title: Option<u64>,
    save_dirs: BTreeMap<u64, std::path::PathBuf>,
}

impl MockSaveFsService {
    pub fn new() -> Self {
        Self {
            mounted_title: None,
            save_dirs: BTreeMap::new(),
        }
    }

    pub fn add_title_saves(&mut self, title_id: u64, dir: std::path::PathBuf) {
        self.save_dirs.insert(title_id, dir);
    }
}

impl SaveFsService for MockSaveFsService {
    fn mount_save(&mut self, title_id: u64) -> Result<(), IpcError> {
        if self.save_dirs.contains_key(&title_id) {
            self.mounted_title = Some(title_id);
            Ok(())
        } else {
            Err(IpcError::MountFailed(alloc::format!(
                "no save data for {title_id:016X}"
            )))
        }
    }

    fn unmount_save(&mut self) {
        self.mounted_title = None;
    }

    fn list_files(&self) -> Result<Vec<SaveFileMeta>, IpcError> {
        let title_id = self
            .mounted_title
            .ok_or_else(|| IpcError::MountFailed(String::from("not mounted")))?;
        let dir = &self.save_dirs[&title_id];

        let mut files = Vec::new();
        for entry in std::fs::read_dir(dir).map_err(|e| IpcError::IoError(alloc::format!("{e}")))? {
            let entry = entry.map_err(|e| IpcError::IoError(alloc::format!("{e}")))?;
            let meta = entry
                .metadata()
                .map_err(|e| IpcError::IoError(alloc::format!("{e}")))?;
            if meta.is_file() {
                files.push(SaveFileMeta {
                    path: entry.file_name().to_string_lossy().into_owned(),
                    size: meta.len(),
                    modified: meta
                        .modified()
                        .ok()
                        .and_then(|t| t.duration_since(std::time::UNIX_EPOCH).ok())
                        .map(|d| d.as_secs())
                        .unwrap_or(0),
                });
            }
        }

        Ok(files)
    }

    fn read_file(&self, path: &str) -> Result<Vec<u8>, IpcError> {
        let title_id = self
            .mounted_title
            .ok_or_else(|| IpcError::MountFailed(String::from("not mounted")))?;
        let dir = &self.save_dirs[&title_id];

        std::fs::read(dir.join(path)).map_err(|e| IpcError::IoError(alloc::format!("{e}")))
    }
}

#[cfg(test)]
#[allow(clippy::disallowed_methods)]
mod tests {
    use super::*;

    #[test]
    fn mock_title_no_game() {
        let svc = MockTitleService { running: None };
        assert!(svc.get_running_application().unwrap().is_none());
    }

    #[test]
    fn mock_title_with_game() {
        let svc = MockTitleService {
            running: Some((1, 0x01007EF00011E000)),
        };
        let (pid, tid) = svc.get_running_application().unwrap().unwrap();
        assert_eq!(pid, 1);
        assert_eq!(tid, 0x01007EF00011E000);
    }

    #[test]
    fn mock_power() {
        let svc = MockPowerService { charging: true };
        assert!(svc.is_charging().unwrap());
    }

    #[test]
    fn mock_save_fs_roundtrip() {
        let dir = tempfile::tempdir().unwrap();
        std::fs::write(dir.path().join("save.dat"), b"hello").unwrap();

        let mut svc = MockSaveFsService::new();
        svc.add_title_saves(0x1234, dir.path().to_path_buf());

        svc.mount_save(0x1234).unwrap();
        let files = svc.list_files().unwrap();
        assert_eq!(files.len(), 1);
        assert_eq!(files[0].path, "save.dat");

        let data = svc.read_file("save.dat").unwrap();
        assert_eq!(data, b"hello");
    }

    #[test]
    fn mock_save_fs_unmounted_errors() {
        let svc = MockSaveFsService::new();
        assert!(svc.list_files().is_err());
    }
}
