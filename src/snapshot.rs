use alloc::format;
use alloc::string::String;
use alloc::vec::Vec;

use crate::config::NxConfig;
use crate::ipc::SaveFsService;

pub fn snapshot_changed_files(
    save_svc: &impl SaveFsService,
    snapshot_dir: &str,
    changed_files: &[String],
) -> usize {
    changed_files
        .iter()
        .filter(|path| snapshot_file(save_svc, snapshot_dir, path))
        .count()
}

fn snapshot_file(save_svc: &impl SaveFsService, snapshot_dir: &str, file_path: &str) -> bool {
    let Ok(data) = save_svc.read_file(file_path) else {
        return false;
    };

    let dest = format!("{snapshot_dir}/{file_path}");
    crate::safe_writer::safe_write(&dest, &data).is_ok()
}

pub fn snapshot_dir(config: &NxConfig, hex_id: &str) -> String {
    let name = config.title_name(hex_id);
    format!("{}/{hex_id}_{name}", config.snapshot_base)
}

#[cfg(test)]
mod tests {
    use super::*;
    use alloc::collections::BTreeMap;

    #[test]
    fn snapshot_dir_with_override() {
        let mut overrides = BTreeMap::new();
        overrides.insert(String::from("01007EF00011E000"), String::from("Zelda BOTW"));
        let config = NxConfig::new().with_title_overrides(overrides);

        let dir = snapshot_dir(&config, "01007EF00011E000");
        assert!(dir.contains("01007EF00011E000_Zelda BOTW"));
    }

    #[test]
    fn snapshot_dir_without_override() {
        let config = NxConfig::new();
        let dir = snapshot_dir(&config, "000000000000DEAD");
        assert!(dir.contains("000000000000DEAD_000000000000DEAD"));
    }
}
