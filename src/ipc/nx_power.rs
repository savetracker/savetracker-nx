use alloc::string::String;
use nx::service;
use nx::service::psm::PsmService;

use super::{IpcError, PowerService};

/// Power service using psm.
///
/// The nx crate's psm trait only exposes get_battery_charge_percentage (cmd 0).
/// Proper charging detection requires GetChargerType (cmd 1) which needs a
/// custom IPC trait definition. Until that's added, is_charging always returns
/// true, effectively disabling pause_on_battery. Users should leave
/// pause_on_battery = false (the default) until this is implemented.
pub struct NxPowerService {
    _psm: PsmService,
}

impl NxPowerService {
    pub fn new() -> Result<Self, IpcError> {
        let psm = service::new_service_object::<PsmService>()
            .map_err(|_| IpcError::ServiceNotAvailable(String::from("psm")))?;
        Ok(Self { _psm: psm })
    }
}

impl PowerService for NxPowerService {
    fn is_charging(&self) -> Result<bool, IpcError> {
        // Cannot detect charging state without GetChargerType (psm cmd 1).
        // Always report charging so pause_on_battery has no effect.
        Ok(true)
    }
}
