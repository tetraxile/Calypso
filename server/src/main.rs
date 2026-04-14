mod server;
mod config;

use slint::{ComponentHandle, spawn_local};

slint::include_modules!();

fn main() {
  let window = MainWindow::new().unwrap();
  {
    let window = window.as_weak();
    window.unwrap().set_log("Hello world".into());
  }
  window.run().unwrap();
}
