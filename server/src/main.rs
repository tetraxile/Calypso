mod config;
mod input_display;
mod script_sender;
mod server;
mod tracked_value;

use std::{
	cell::RefCell,
	panic::{AssertUnwindSafe, catch_unwind},
	path::PathBuf,
	rc::Rc,
	sync::{Arc, Mutex},
};

use eyre::{Context, Result, bail};
use slint::{ComponentHandle, Image, Rgba8Pixel, SharedPixelBuffer, SharedString, VecModel};
use tas_script_formats::{Script, glam::Vec3};
use tiny_skia::PixmapMut;
use tokio::sync::mpsc;
use tracing::info;

use crate::{
	config::Config,
	input_display::InputDisplay,
	script_sender::{ScriptMessage, script_sender},
	server::{ToServer, server_task},
	tracked_value::TrackedValue,
};

slint::include_modules!();

struct ActiveScript {
	path: PathBuf,
	script: Arc<Script>,
}

struct State {
	active_script: TrackedValue<Option<ActiveScript>>,
	script_listener: mpsc::Sender<ScriptMessage>,

	log: TrackedValue<String>,
	input_display: InputDisplay,
	config: Config,

	stage: TrackedValue<(String, i32)>,
	position: TrackedValue<Vec3>,
}

impl State {
	pub fn apply_changes(&mut self, window: &MainWindow, updater: impl FnOnce(&mut State)) {
		updater(self);

		self.active_script.if_changed(|script| {
			window.set_script_name(
				script
					.as_ref()
					.map(|script| {
						script
							.path
							.file_name()
							.unwrap()
							.to_string_lossy()
							.into_owned()
							.into()
					})
					.unwrap_or_default(),
			);
			window.set_author(
				script
					.as_ref()
					.and_then(|script| script.script.author.to_owned())
					.map(|author| author.into())
					.unwrap_or_default(),
			);
			window.set_frame_count(
				script
					.as_ref()
					.map(|script| script.script.frames.len() as _)
					.unwrap_or(0),
			);
			match script {
				Some(script) => self
					.script_listener
					.blocking_send(ScriptMessage::Script(script.script.clone())),
				None => self
					.script_listener
					.blocking_send(ScriptMessage::Stop { manual: false }),
			}
			.expect("script sender closed");
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

		self.stage.if_changed(|(name, scenario)| {
			window.set_stage_name(name.into());
			window.set_scenario(*scenario as _);
		});

		self.position.if_changed(|position| {
			window.set_position((position.x, position.y, position.z));
		});

		if update_config {
			self.config.save();
		}
	}
}

fn try_loading_script(path: &PathBuf, log: &mut String) -> Result<ActiveScript> {
	let result = std::fs::read(path)
		.context("failed to read script file from disk")
		.and_then(|script| tas_script_formats::parse(&script));
	match result {
		Ok(script) => Ok(ActiveScript {
			path: path.clone(),
			script: script.into(),
		}),
		Err(report) => {
			log.push_str("server: failed to load script\n");
			log.push_str(&report.to_string());
			log.push('\n');
			bail!("failed to load script");
		}
	}
}

fn select_script(state: &mut State, file: PathBuf) {
	if !state
		.config
		.recent_scripts
		.get()
		.iter()
		.any(|other| other == &file)
	{
		let recent_scripts = state.config.recent_scripts.get_mut();
		recent_scripts.retain(|path| path != &file);
		recent_scripts.insert(0, file.clone());
	}
}

fn main() {
	tracing_subscriber::fmt().init();

	let window = MainWindow::new().unwrap();
	let config = Config::load();
	let (to_ui, mut from_server) = mpsc::unbounded_channel();
	let (to_server, from_ui) = mpsc::unbounded_channel();
	let (to_script_manager, from_state) = mpsc::channel(4);
	let state = Rc::new(RefCell::new(State {
		active_script: None.into(),
		script_listener: to_script_manager.clone(),
		log: String::new().into(),
		input_display: InputDisplay::default().into(),
		config,
		stage: ("Not in a stage".to_owned(), 0).into(),
		position: Vec3::ZERO.into(),
	}));

	{
		// load last used script from disk
		let mut state = state.borrow_mut();
		let script = state
			.config
			.recent_scripts
			.get()
			.get(0)
			.cloned()
			.and_then(|path| try_loading_script(&path, state.log.get_mut()).ok());
		state.active_script.set(script);
	};

	// apply initial state changes including config and script
	state.borrow_mut().apply_changes(&window, |_| {});

	let window_weak = window.as_weak();
	let state_ = state.clone();
	window.on_open_script_dialog(move || {
		let state = &state_;
		let window = window_weak.unwrap();

		let fd = rfd::FileDialog::new()
			.set_title("Open TAS script")
			.set_parent(&window.window().window_handle());

		if let Some(file) = fd.pick_file() {
			state.borrow_mut().apply_changes(&window, |state| {
				select_script(state, file);
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

	window.on_run_script({
		let to_script_manager = to_script_manager.clone();
		move || {
			to_script_manager
				.blocking_send(ScriptMessage::Start)
				.expect("channel closed")
		}
	});

	window.on_stop_script({
		let to_script_manager = to_script_manager.clone();
		move || {
			to_script_manager
				.blocking_send(ScriptMessage::Stop { manual: true })
				.expect("channel closed")
		}
	});
	window.on_pause_game({
		let to_server = to_server.clone();
		move || to_server.send(ToServer::PauseGame).expect("channel closed")
	});

	window.on_frame_advance({
		let to_server = to_server.clone();
		move || {
			to_server
				.send(ToServer::AdvanceFrame)
				.expect("channel closed")
		}
	});

	let handle = std::thread::spawn(move || {
		let runtime = tokio::runtime::Builder::new_current_thread()
			.enable_all()
			.build()
			.unwrap();
		let _guard = runtime.enter();
		tokio::spawn(script_sender(from_state, to_ui.clone(), to_server.clone()));
		runtime.block_on(server_task(to_ui, from_ui));
		panic!("server closed")
	});
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
						state.log.get_mut().push('\n');
					});
				}
				server::ToUi::ClientConnected => {
					if let Some(script) = state.borrow().active_script.get() {
						to_script_manager
							.blocking_send(ScriptMessage::Script(script.script.clone()))
							.expect("channel closed");
					}
				}
				server::ToUi::ClientError(report) => {
					state
						.borrow_mut()
						.log
						.get_mut()
						.push_str(&format!("{}\n", report));
				}
				server::ToUi::SaveFile(_) => {}
				server::ToUi::ReportPosition { position } => {
					state.borrow_mut().apply_changes(&window, |state| {
						state.position.set(position);
					});
				}
				server::ToUi::ReportStage {
					stage_name,
					scenario,
				} => {
					state.borrow_mut().apply_changes(&window, |state| {
						state.stage.set((stage_name.into(), scenario));
					});
				}
				server::ToUi::FullFrameBuffer { server_index } => {
					info!("backoff");
					to_script_manager
						.blocking_send(ScriptMessage::BackOff { server_index })
						.expect("channel closed");
				}
				server::ToUi::ScriptPlaybackEnded => {
					info!("playback ended");
					to_script_manager
						.blocking_send(ScriptMessage::Stop { manual: false })
						.expect("channel closed");
				}
			}
		}
	})
	.unwrap();

	catch_unwind(AssertUnwindSafe(|| {
		window.run().unwrap();
	}));
	handle.join();
}
