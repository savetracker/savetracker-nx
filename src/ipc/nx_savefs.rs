//! Concrete SaveFsService using fsp-srv IPC service.
//! TODO: implement against actual nx crate API — current stubs need hardware validation.

use alloc::string::String;
use alloc::vec::Vec;

use super::{IpcError, SaveFileMeta, SaveFsService};

pub struct NxSaveFsService;

impl NxSaveFsService {
    pub fn new() -> Result<Self, IpcError> {
        // TODO: connect to fsp-srv service
        Ok(Self)
    }
}

impl SaveFsService for NxSaveFsService {
    fn mount_save(&mut self, _title_id: u64) -> Result<(), IpcError> {
        // TODO: call fsp-srv OpenSaveDataFileSystem
        Ok(())
    }

    fn unmount_save(&mut self) {
        // TODO: close mounted filesystem
    }

    fn list_files(&self) -> Result<Vec<SaveFileMeta>, IpcError> {
        // TODO: enumerate files in mounted save filesystem
        Ok(Vec::new())
    }

    fn read_file(&self, _path: &str) -> Result<Vec<u8>, IpcError> {
        // TODO: read file from mounted save filesystem
        Err(IpcError::MountFailed(String::from("not yet implemented")))
    }
}
