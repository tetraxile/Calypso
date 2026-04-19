use std::{fs, path::PathBuf, sync::LazyLock};

use serde::{Deserialize, Serialize};
use tracing::{error, info};

use crate::tracked_value::TrackedValue;

static PROJECT_DIR: LazyLock<directories::ProjectDirs> = LazyLock::new(|| {
	directories::ProjectDirs::from("dev.tetraxile", "tetraxile", "Calypso").unwrap()
});

#[derive(Serialize, Deserialize, Default)]
pub struct Config {
	pub recent_scripts: TrackedValue<Vec<PathBuf>>,
}

impl Config {
	fn config_path() -> PathBuf {
		let _ = fs::create_dir_all(PROJECT_DIR.config_dir())
			.inspect_err(|error| error!("failed to create config directory: {error}"));
		PROJECT_DIR.config_dir().join("config.toml")
	}

	pub fn save(&self) {
		let config_path = Self::config_path();
		if let Err(error) = fs::write(
			&config_path,
			toml::to_string_pretty(self).expect("failed to serialize to toml"),
		) {
			error!("failed to save config to {config_path:?}: {error}");
		} else {
			info!("saved config!");
		}
	}

	pub fn load() -> Self {
		let config_path = Self::config_path();
		let Ok(toml_text) = fs::read_to_string(&config_path) else {
			error!("failed to read config at {config_path:?}");
			return Config::default();
		};

		toml::from_str(&toml_text).unwrap_or_default()
	}
}
