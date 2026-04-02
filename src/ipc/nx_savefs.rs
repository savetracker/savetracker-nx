use alloc::format;
use alloc::string::String;
use alloc::vec;
use alloc::vec::Vec;

use nx::fs;
use nx::fs::{DirectoryEntryType, DirectoryOpenMode, FileOpenOption};

use super::{IpcError, SaveFileMeta, SaveFsService};

pub struct NxSaveFsService {
    mount_path: Option<String>,
}

impl NxSaveFsService {
    pub fn new() -> Result<Self, IpcError> {
        fs::mount_sd_card("sd").map_err(|e| {
            IpcError::MountFailed(format!("failed to mount SD card: {}", e.get_value()))
        })?;
        Ok(Self { mount_path: None })
    }
}

impl SaveFsService for NxSaveFsService {
    fn mount_save(&mut self, title_id: u64) -> Result<(), IpcError> {
        // Look for saves in Checkpoint's export directory on SD
        let path = format!("sd:/switch/Checkpoint/saves/{title_id:016X}");

        fs::open_directory(&path, DirectoryOpenMode::ReadFiles())
            .map_err(|e| IpcError::MountFailed(format!("no saves at {path}: {}", e.get_value())))?;

        self.mount_path = Some(path);
        Ok(())
    }

    fn unmount_save(&mut self) {
        self.mount_path = None;
    }

    fn list_files(&self) -> Result<Vec<SaveFileMeta>, IpcError> {
        let base = self
            .mount_path
            .as_ref()
            .ok_or_else(|| IpcError::MountFailed(String::from("not mounted")))?;

        let mut dir = fs::open_directory(base, DirectoryOpenMode::ReadFiles())
            .map_err(|e| IpcError::IoError(format!("open_directory: {}", e.get_value())))?;

        let mut files = Vec::new();
        while let Ok(Some(entry)) = dir.read_next() {
            if entry.entry_type == DirectoryEntryType::File {
                let name = entry
                    .name
                    .get_str()
                    .map_err(|_| IpcError::IoError(String::from("invalid filename")))?;

                files.push(SaveFileMeta {
                    path: String::from(name),
                    size: entry.file_size as u64,
                    modified: 0,
                });
            }
        }

        Ok(files)
    }

    fn read_file(&self, path: &str) -> Result<Vec<u8>, IpcError> {
        let base = self
            .mount_path
            .as_ref()
            .ok_or_else(|| IpcError::MountFailed(String::from("not mounted")))?;

        let full = format!("{base}/{path}");
        let mut file = fs::open_file(&full, FileOpenOption::Read())
            .map_err(|e| IpcError::IoError(format!("open {full}: {}", e.get_value())))?;

        let size = file
            .get_size()
            .map_err(|e| IpcError::IoError(format!("get_size: {}", e.get_value())))?;

        let mut buf: Vec<u8> = vec![0u8; size];
        file.read_array(&mut buf)
            .map_err(|e| IpcError::IoError(format!("read: {}", e.get_value())))?;

        Ok(buf)
    }
}
