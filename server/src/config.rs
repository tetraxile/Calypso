use std::path::PathBuf;

use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize)]
struct Config {
  recent_scripts: Vec<PathBuf>,
}
