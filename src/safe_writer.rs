use alloc::string::String;
use alloc::vec::Vec;

const ALLOWED_ROOTS: &[&str] = &[
    "sdmc:/switch/savetracker",
    "sdmc:/config/savetracker",
];

fn apply_segment<'a>(parts: &mut Vec<&'a str>, segment: &'a str) {
    match segment {
        ".." => { parts.pop(); }
        "." | "" => {}
        other => parts.push(other),
    }
}

fn normalize(path: &str) -> String {
    let unified = path.replace('\\', "/");
    let mut parts: Vec<&str> = Vec::new();

    for segment in unified.split('/') {
        apply_segment(&mut parts, segment);
    }

    parts.join("/")
}

fn is_allowed(normalized: &str) -> bool {
    ALLOWED_ROOTS.iter().any(|root| normalized.starts_with(root))
}

pub fn get_safe_path(path: &str) -> String {
    let normalized = normalize(path);
    assert!(
        is_allowed(&normalized),
        "path not in allowed roots: {normalized}",
    );
    normalized
}

#[cfg(not(target_os = "horizon"))]
pub fn safe_write(path: &str, data: &[u8]) -> Result<(), String> {
    // Desktop stub — tests don't write to sdmc: paths
    Err(alloc::format!("safe_write not available on desktop: {path}"))
}

#[cfg(target_os = "horizon")]
pub fn safe_write(path: &str, data: &[u8]) -> Result<(), String> {
    use nx::fs::{self, FileAttribute, FileOpenOption};

    let path = get_safe_path(path);

    if let Some(last_slash) = path.rfind('/') {
        let parent = &path[..last_slash];
        let _ = fs::create_directory(parent);
    }

    let _ = fs::remove_file(&path);
    fs::create_file(&path, data.len(), FileAttribute::None())
        .map_err(|e| alloc::format!("create_file {path}: {}", e.get_value()))?;

    let mut file = fs::open_file(&path, FileOpenOption::Write())
        .map_err(|e| alloc::format!("open_file {path}: {}", e.get_value()))?;

    file.write_array::<_, true>(data)
        .map_err(|e| alloc::format!("write {path}: {}", e.get_value()))?;

    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn normalizes_dot_segments() {
        let result = normalize("sdmc:/switch/savetracker/./sub/../file.txt");
        assert_eq!(result, "sdmc:/switch/savetracker/file.txt");
    }

    #[test]
    fn allowed_snapshot_path() {
        let result = get_safe_path("sdmc:/switch/savetracker/01234/save.dat");
        assert_eq!(result, "sdmc:/switch/savetracker/01234/save.dat");
    }

    #[test]
    fn allowed_config_path() {
        let result = get_safe_path("sdmc:/config/savetracker/config.toml");
        assert_eq!(result, "sdmc:/config/savetracker/config.toml");
    }

    #[test]
    #[should_panic(expected = "path not in allowed roots")]
    fn rejects_traversal_out_of_root() {
        get_safe_path("sdmc:/switch/savetracker/../../Nintendo/test");
    }

    #[test]
    #[should_panic(expected = "path not in allowed roots")]
    fn rejects_save_directory() {
        get_safe_path("sdmc:/Nintendo/save.dat");
    }

    #[test]
    #[should_panic(expected = "path not in allowed roots")]
    fn rejects_arbitrary_path() {
        get_safe_path("sdmc:/other/file.txt");
    }
}
