use alloc::collections::BTreeMap;
use alloc::string::String;
use alloc::vec::Vec;

use serde::Deserialize;

const DEFAULT_SNAPSHOT_BASE: &str = "sdmc:/switch/savetracker";
const DEFAULT_MAX_SNAPSHOTS: usize = 50;
const DEFAULT_POLL_SAVE_INTERVAL: u64 = 5;
const DEFAULT_POLL_TITLE_INTERVAL: u64 = 30;
const DEFAULT_POLL_POWER_INTERVAL: u64 = 180;
const DEFAULT_IDLE_ON_NO_TITLE: u64 = 90;

#[derive(Debug, Clone, Deserialize)]
#[serde(from = "RawNxConfig")]
pub struct NxConfig {
    pub snapshot_base: String,
    pub max_snapshots: usize,
    pub poll_save_interval: u64,
    pub poll_title_interval: u64,
    pub poll_power_interval: u64,
    pub idle_on_no_title: u64,
    pub pause_on_battery: bool,
    pub forced_format: Option<String>,
    pub format_params: BTreeMap<String, String>,
    pub title_overrides: BTreeMap<String, String>,
    pub exclude_titles: Vec<String>,
}

#[derive(Deserialize)]
struct RawNxConfig {
    snapshot_base: Option<String>,
    max_snapshots: Option<usize>,
    poll_save_interval: Option<u64>,
    poll_title_interval: Option<u64>,
    poll_power_interval: Option<u64>,
    idle_on_no_title: Option<u64>,
    #[serde(default)]
    pause_on_battery: bool,
    forced_format: Option<String>,
    #[serde(default)]
    format_params: BTreeMap<String, String>,
    #[serde(default)]
    title_overrides: BTreeMap<String, String>,
    #[serde(default)]
    exclude_titles: Vec<String>,
}

impl From<RawNxConfig> for NxConfig {
    fn from(raw: RawNxConfig) -> Self {
        NxConfig::new()
            .with_snapshot_base(raw.snapshot_base.unwrap_or_else(|| String::from(DEFAULT_SNAPSHOT_BASE)))
            .with_max_snapshots(raw.max_snapshots.unwrap_or(DEFAULT_MAX_SNAPSHOTS))
            .with_poll_save_interval(raw.poll_save_interval.unwrap_or(DEFAULT_POLL_SAVE_INTERVAL))
            .with_poll_title_interval(raw.poll_title_interval.unwrap_or(DEFAULT_POLL_TITLE_INTERVAL))
            .with_poll_power_interval(raw.poll_power_interval.unwrap_or(DEFAULT_POLL_POWER_INTERVAL))
            .with_idle_on_no_title(raw.idle_on_no_title.unwrap_or(DEFAULT_IDLE_ON_NO_TITLE))
            .with_pause_on_battery(raw.pause_on_battery)
            .with_forced_format(raw.forced_format)
            .with_format_params(raw.format_params)
            .with_title_overrides(raw.title_overrides)
            .with_exclude_titles(raw.exclude_titles)
    }
}

impl NxConfig {
    pub fn new() -> Self {
        Self {
            snapshot_base: String::from(DEFAULT_SNAPSHOT_BASE),
            max_snapshots: DEFAULT_MAX_SNAPSHOTS,
            poll_save_interval: DEFAULT_POLL_SAVE_INTERVAL,
            poll_title_interval: DEFAULT_POLL_TITLE_INTERVAL,
            poll_power_interval: DEFAULT_POLL_POWER_INTERVAL,
            idle_on_no_title: DEFAULT_IDLE_ON_NO_TITLE,
            pause_on_battery: false,
            forced_format: None,
            format_params: BTreeMap::new(),
            title_overrides: BTreeMap::new(),
            exclude_titles: Vec::new(),
        }
    }

    #[cfg(target_os = "horizon")]
    pub fn load() -> Self {
        use nx::fs::{self, FileOpenOption};

        let path = "sd:/config/savetracker/config.toml";
        let mut file = match fs::open_file(path, FileOpenOption::Read()) {
            Ok(f) => f,
            Err(_) => return Self::new(),
        };

        let size = match file.get_size() {
            Ok(s) => s,
            Err(_) => return Self::new(),
        };

        let mut buf = alloc::vec![0u8; size];
        if file.read_array(&mut buf).is_err() {
            return Self::new();
        }

        let content = match core::str::from_utf8(&buf) {
            Ok(s) => s,
            Err(_) => return Self::new(),
        };

        toml::from_str(content).unwrap_or_default()
    }

    pub fn with_snapshot_base(mut self, base: String) -> Self {
        self.snapshot_base = base;
        self
    }

    pub fn with_max_snapshots(mut self, max: usize) -> Self {
        self.max_snapshots = max;
        self
    }

    pub fn with_poll_save_interval(mut self, secs: u64) -> Self {
        self.poll_save_interval = secs;
        self
    }

    pub fn with_poll_title_interval(mut self, secs: u64) -> Self {
        self.poll_title_interval = secs;
        self
    }

    pub fn with_poll_power_interval(mut self, secs: u64) -> Self {
        self.poll_power_interval = secs;
        self
    }

    pub fn with_idle_on_no_title(mut self, secs: u64) -> Self {
        self.idle_on_no_title = secs;
        self
    }

    pub fn with_pause_on_battery(mut self, pause: bool) -> Self {
        self.pause_on_battery = pause;
        self
    }

    pub fn with_forced_format(mut self, format: Option<String>) -> Self {
        self.forced_format = format;
        self
    }

    pub fn with_format_params(mut self, params: BTreeMap<String, String>) -> Self {
        self.format_params = params;
        self
    }

    pub fn with_title_overrides(mut self, overrides: BTreeMap<String, String>) -> Self {
        self.title_overrides = overrides;
        self
    }

    pub fn with_exclude_titles(mut self, titles: Vec<String>) -> Self {
        self.exclude_titles = titles;
        self
    }

    pub fn title_name(&self, hex_id: &str) -> String {
        self.title_overrides
            .get(hex_id)
            .cloned()
            .unwrap_or_else(|| String::from(hex_id))
    }

    pub fn is_excluded(&self, hex_id: &str) -> bool {
        self.exclude_titles.iter().any(|e| e == hex_id)
    }
}

impl Default for NxConfig {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn default_config() {
        let config = NxConfig::new();
        assert_eq!(config.snapshot_base, "sdmc:/switch/savetracker");
        assert_eq!(config.max_snapshots, 50);
        assert_eq!(config.poll_save_interval, 5);
    }

    #[test]
    fn builder_overrides() {
        let config = NxConfig::new()
            .with_max_snapshots(100)
            .with_pause_on_battery(true);

        assert_eq!(config.max_snapshots, 100);
        assert!(config.pause_on_battery);
    }

    #[test]
    fn title_override_lookup() {
        let mut overrides = BTreeMap::new();
        overrides.insert(String::from("01007EF00011E000"), String::from("Zelda BOTW"));
        let config = NxConfig::new().with_title_overrides(overrides);

        assert_eq!(config.title_name("01007EF00011E000"), "Zelda BOTW");
        assert_eq!(config.title_name("00000000DEADBEEF"), "00000000DEADBEEF");
    }

    #[test]
    fn exclude_titles() {
        let config = NxConfig::new()
            .with_exclude_titles(alloc::vec![String::from("0100000000010000")]);

        assert!(config.is_excluded("0100000000010000"));
        assert!(!config.is_excluded("01007EF00011E000"));
    }
}
