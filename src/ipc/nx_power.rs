//! Concrete PowerService using psm IPC service.
//! TODO: implement against actual nx crate API — current stubs need hardware validation.

use super::{IpcError, PowerService};

pub struct NxPowerService;

impl NxPowerService {
    pub fn new() -> Result<Self, IpcError> {
        // TODO: connect to psm service
        Ok(Self)
    }
}

impl PowerService for NxPowerService {
    fn is_charging(&self) -> Result<bool, IpcError> {
        // TODO: call psm GetChargerType
        Ok(true)
    }
}
