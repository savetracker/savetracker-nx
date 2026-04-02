#[cfg(not(target_os = "horizon"))]
pub mod mock;

#[cfg(target_os = "horizon")]
pub mod nx_power;
#[cfg(target_os = "horizon")]
pub mod nx_savefs;
#[cfg(target_os = "horizon")]
pub mod nx_title;

use alloc::string::String;
use alloc::vec::Vec;
use core::fmt;

#[derive(Debug)]
pub enum IpcError {
    ServiceNotAvailable(String),
    CommandFailed { service: String, code: u32 },
    NoApplicationRunning,
    MountFailed(String),
    IoError(String),
}

impl fmt::Display for IpcError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::ServiceNotAvailable(name) => write!(f, "service unavailable: {name}"),
            Self::CommandFailed { service, code } => {
                write!(f, "{service} command failed: {code:#X}")
            }
            Self::NoApplicationRunning => write!(f, "no application running"),
            Self::MountFailed(reason) => write!(f, "mount failed: {reason}"),
            Self::IoError(e) => write!(f, "io error: {e}"),
        }
    }
}

#[derive(Debug, Clone)]
pub struct SaveFileMeta {
    pub path: String,
    pub size: u64,
    pub modified: u64,
}

pub trait TitleService {
    fn get_running_application(&self) -> Result<Option<(u64, u64)>, IpcError>;
}

pub trait PowerService {
    fn is_charging(&self) -> Result<bool, IpcError>;
}

pub trait SaveFsService {
    fn mount_save(&mut self, title_id: u64) -> Result<(), IpcError>;
    fn unmount_save(&mut self);
    fn list_files(&self) -> Result<Vec<SaveFileMeta>, IpcError>;
    fn read_file(&self, path: &str) -> Result<Vec<u8>, IpcError>;
}
