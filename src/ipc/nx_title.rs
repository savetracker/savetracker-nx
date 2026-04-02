//! Concrete TitleService using pm:dmnt and pm:info IPC services.
//! TODO: implement against actual nx crate API — current stubs need hardware validation.

use super::{IpcError, TitleService};

pub struct NxTitleService;

impl NxTitleService {
    pub fn new() -> Result<Self, IpcError> {
        // TODO: connect to pm:dmnt and pm:info services
        Ok(Self)
    }
}

impl TitleService for NxTitleService {
    fn get_running_application(&self) -> Result<Option<(u64, u64)>, IpcError> {
        // TODO: call pm:dmnt GetApplicationProcessId + pm:info GetProgramId
        Ok(None)
    }
}
