use alloc::string::String;
use nx::ipc::sf::pm::IDebugMonitorInterfaceClient;
use nx::ipc::sf::pm::IInformationInterfaceClient;
use nx::service;
use nx::service::pm::DebugMonitorInterfaceService;
use nx::service::pm::InformationInterfaceService;

use super::{IpcError, TitleService};

pub struct NxTitleService {
    dmnt: DebugMonitorInterfaceService,
    info: InformationInterfaceService,
}

impl NxTitleService {
    pub fn new() -> Result<Self, IpcError> {
        let dmnt = service::new_service_object::<DebugMonitorInterfaceService>()
            .map_err(|_| IpcError::ServiceNotAvailable(String::from("pm:dmnt")))?;
        let info = service::new_service_object::<InformationInterfaceService>()
            .map_err(|_| IpcError::ServiceNotAvailable(String::from("pm:info")))?;
        Ok(Self { dmnt, info })
    }
}

impl TitleService for NxTitleService {
    fn get_running_application(&self) -> Result<Option<(u64, u64)>, IpcError> {
        let pid = match self.dmnt.get_application_process_id() {
            Ok(pid) => pid,
            Err(_) => return Ok(None),
        };

        let program_id = self
            .info
            .get_program_id(pid)
            .map_err(|e| IpcError::CommandFailed {
                service: String::from("pm:info"),
                code: e.get_value(),
            })?;

        Ok(Some((pid, program_id.0)))
    }
}
