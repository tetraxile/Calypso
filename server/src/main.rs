mod config;
mod input_display;
mod server;
mod tracked_value;

use std::{cell::RefCell, path::PathBuf, rc::Rc, sync::Mutex};

use slint::{ComponentHandle, Image, Rgba8Pixel, SharedPixelBuffer, SharedString, VecModel};
use tiny_skia::PixmapMut;
use tokio::sync::mpsc;

use crate::{
	config::Config, input_display::InputDisplay, server::server_task, tracked_value::TrackedValue,
};

slint::include_modules!();

struct State {
	active_script: TrackedValue<Option<PathBuf>>,
	log: TrackedValue<String>,
	input_display: InputDisplay,
	config: Config,
}

impl State {
	pub fn apply_changes(&mut self, window: &MainWindow, updater: impl FnOnce(&mut State)) {
		updater(self);

		self.active_script.if_changed(|script| {
			window.set_script_name(
				script
					.to_owned()
					.map(|script| {
						script
							.file_name()
							.unwrap()
							.to_string_lossy()
							.into_owned()
							.into()
					})
					.unwrap_or_default(),
			);
		});

		self.log.if_changed(|log| {
			window.set_log(log.as_str().into());
		});

		let mut update_config = false;
		self.config.recent_scripts.if_changed(|recents| {
			window.set_recent_scripts(
				Rc::new(VecModel::from(
					recents
						.iter()
						.map(|path| path.to_string_lossy().into_owned().into())
						.collect::<Vec<SharedString>>(),
				))
				.into(),
			);
			update_config = true;
		});

		if update_config {
			self.config.save();
		}
	}
}

fn main() {
	tracing_subscriber::fmt().init();

	let window = MainWindow::new().unwrap();
	let config = Config::load();
	let state = Rc::new(RefCell::new(State {
		active_script: config.recent_scripts.get().get(0).cloned().into(),
		log: String::new().into(),
		input_display: InputDisplay::default().into(),
		config,
	}));

	state.borrow_mut().apply_changes(&window, |_| {});

	let window_weak = window.as_weak();
	let state_ = state.clone();
	window.on_open_script_dialog(move || {
		let state = &state_;
		println!("test");
		let window = window_weak.unwrap();

		let fd = rfd::FileDialog::new()
			.set_title("Open TAS script")
			.set_parent(&window.window().window_handle());

		if let Some(file) = fd.pick_file() {
			state.borrow_mut().apply_changes(&window, |state| {
				if !state
					.config
					.recent_scripts
					.get()
					.iter()
					.any(|other| other == &file)
				{
					state
						.config
						.recent_scripts
						.get_mut()
						.insert(0, file.clone());
				}
				state.active_script.set(Some(file));
			});
		} else {
			println!("war");
		}
	});

	let state_ = state.clone();
	window.on_build_image(move |width, height, _input_sequence, scheme| {
		let state = &state_;
		static BUFFER: Mutex<Option<SharedPixelBuffer<Rgba8Pixel>>> = Mutex::new(None);
		let mut buffer_guard = BUFFER.lock().expect("lock poisoned");

		buffer_guard
			.take_if(|buffer| buffer.width() != width as u32 || buffer.height() != height as u32);

		let buffer = buffer_guard
			.get_or_insert_with(|| SharedPixelBuffer::<Rgba8Pixel>::new(width as _, height as _));

		let (width, height) = (buffer.width(), buffer.height());
		state.borrow().input_display.paint(
			PixmapMut::from_bytes(buffer.make_mut_bytes(), width, height)
				.expect("failed to create pixmap"),
			scheme,
		);

		Image::from_rgba8(buffer.clone())
	});

	let (to_ui, mut from_server) = mpsc::unbounded_channel();
	let (_to_server, from_ui) = mpsc::unbounded_channel();

	let _ = slint::spawn_local(async_compat::Compat::new(server_task(to_ui, from_ui)));
	let window_weak = window.as_weak();
	let _ = slint::spawn_local(async move {
		let window = window_weak.unwrap();

		loop {
			let message = from_server
				.recv()
				.await
				.expect("server closed unexpectedly");

			match message {
				server::ToUi::Log(message) => {
					state.borrow_mut().apply_changes(&window, |state| {
						state.log.get_mut().push_str(&message);
					});
				}
			}
		}
	})
	.unwrap();

	window.run().unwrap();
}
