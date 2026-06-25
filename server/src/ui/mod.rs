mod game_info;
mod input_display;
mod script_info;

use std::{fmt::Write as FmtWrite, path::PathBuf, sync::Arc};

use eframe::{
	CreationContext, Frame,
	egui::{FontData, FontDefinitions, FontFamily, FontId, InnerResponse, TextEdit, TextStyle, Ui},
};
use egui_dock::TabViewer;
use eyre::{Context, Result, bail};
use tas_script_formats::{
	Buttons, Script,
	glam::Vec3,
};
use tokio::sync::mpsc;

use crate::{
	config::Config,
	script_sender::{ScriptMessage, script_sender},
	server::{ToServer, ToUi, server_task},
	tracked_value::TrackedValue,
	ui::input_display::InputDisplay,
};

pub enum TabType {
	ScriptInfo,
	InputDisplay,
	GameInfo,
	Log,
}

impl TabType {
	fn name(&self) -> &str {
		match self {
			Self::ScriptInfo => "Script",
			Self::InputDisplay => "Input Display",
			Self::GameInfo => "Game Info",
			Self::Log => "Log",
		}
	}
}

struct ActiveScript {
	path: PathBuf,
	script: Arc<Script>,
}

pub(super) struct State {
	script_sender: mpsc::Sender<ScriptMessage>,
	server_sender: mpsc::UnboundedSender<ToServer>,
	ui_receiver: mpsc::UnboundedReceiver<ToUi>,

	should_open_dialog: bool,
	config: Config,
	active_script: TrackedValue<Option<ActiveScript>>,
	script_frame_text: String,
	log: String,
	input_display: InputDisplay,
	stage: Option<(String, i32)>,
	player_position: Option<Vec3>,
	monospace: FontId,
}

impl State {
	pub fn new(_ctx: &CreationContext) -> Self {
		let (to_ui, mut from_server_repainter) = mpsc::unbounded_channel();
		let (to_server, from_ui) = mpsc::unbounded_channel();
		let (to_script_manager, from_state) = mpsc::channel(4);
		tokio::spawn(server_task(to_ui.clone(), from_ui));
		tokio::spawn(script_sender(from_state, to_ui, to_server.clone()));
		let context = _ctx.egui_ctx.clone();
		let mut font_definitions = FontDefinitions::default();
		let mut font_data = FontData::from_static(include_bytes!("./assets/ComicShanns2.ttf"));
		font_data.tweak.y_offset = 3.0;
		font_definitions
			.font_data
			.insert("ComicShanns2".to_owned(), font_data.into());

		font_definitions
			.families
			.get_mut(&FontFamily::Monospace)
			.unwrap()
			.insert(0, "ComicShanns2".to_owned());
		context.set_fonts(font_definitions);
		let monospace = TextStyle::Monospace.resolve(&context.global_style());

		let (to_ui, from_server) = mpsc::unbounded_channel();
		tokio::spawn(async move {
			while let Some(packet) = from_server_repainter.recv().await {
				context.request_repaint();
				if let Err(_) = to_ui.send(packet) {
					return;
				}
			}
		});

		let mut state = State {
			script_sender: to_script_manager,
			server_sender: to_server,
			ui_receiver: from_server,

			should_open_dialog: false,
			config: Config::load(),
			active_script: None.into(),
			script_frame_text: String::new(),
			log: String::new(),
			input_display: InputDisplay::new(),
			stage: None,
			player_position: None,
			monospace,
		};

		if let Some(most_recent) = state.config.recent_scripts.get().get(0) {
			state.select_script(most_recent.clone());
		}

		state
	}

	fn monospace_scope<R>(
		font_id: FontId,
		ui: &mut Ui,
		func: impl FnOnce(&mut Ui) -> R,
	) -> InnerResponse<R> {
		ui.scope(|ui| {
			ui.style_mut().override_font_id = Some(font_id);
			func(ui)
		})
	}

	pub fn update(&mut self, frame: &mut Frame) {
		let mut update_config = true;

		self.config
			.recent_scripts
			.if_changed(|_| update_config = true);

		if update_config {
			self.config.save();
		}

		if self.should_open_dialog {
			self.should_open_dialog = false;
			let fd = rfd::FileDialog::new()
				.set_title("Open TAS script")
				.set_parent(frame);

			if let Some(file) = fd.pick_file() {
				self.select_script(file);
			}
		}

		self.active_script.if_changed(|script| {
			self.script_frame_text = match script {
				Some(script) => script.script.frames.len().to_string(),
				None => String::new(),
			}
		});

		while let Ok(message) = self.ui_receiver.try_recv() {
			match message {
				ToUi::Log(log_message) => {
					writeln!(&mut self.log, "{log_message}").unwrap();
				}
				ToUi::SaveFile(_) => {}
				ToUi::ClientConnected => {
					if let Some(script) = self.active_script.get() {
						self.script_sender
							.blocking_send(ScriptMessage::Script(script.script.clone()))
							.expect("channel closed");
					}
				}
				ToUi::ClientError(report) => {
					self.log.push_str(&format!("{}\n", report));
				}
				ToUi::FullFrameBuffer { server_index } => self
					.script_sender
					.blocking_send(ScriptMessage::BackOff { server_index })
					.unwrap(),
				ToUi::ScriptPlaybackEnded => self
					.script_sender
					.blocking_send(ScriptMessage::Stop { manual: false })
					.unwrap(),
				ToUi::ReportStage {
					stage_name,
					scenario,
				} => self.stage = Some((stage_name, scenario)),
				ToUi::ReportPosition { position } => self.player_position = Some(position),
				ToUi::InputReport(report) => {
					self.input_display.update(
						Buttons::new(),
						report.left_stick.with_y(-report.left_stick.y),
						report.right_stick.with_y(-report.right_stick.y),
					);
				}
			}
		}
	}

	fn try_loading_script(&mut self, path: &PathBuf) -> Result<ActiveScript> {
		let result = std::fs::read(path)
			.wrap_err("failed to read script file from disk")
			.and_then(|script| tas_script_formats::parse(&script));

		match result {
			Ok(script) => Ok(ActiveScript {
				path: path.clone(),
				script: script.into(),
			}),
			Err(report) => {
				let _ = writeln!(&mut self.log, "server: failed to load script");
				let _ = writeln!(&mut self.log, "{report}");
				bail!("failed to load script");
			}
		}
	}

	fn select_script(&mut self, file: PathBuf) {
		if let Ok(script) = self.try_loading_script(&file) {
			let recent_scripts = self.config.recent_scripts.get_mut();
			recent_scripts.retain(|path| path != &file);
			recent_scripts.insert(0, file);
			self.script_sender
				.blocking_send(ScriptMessage::Script(script.script.clone()))
				.unwrap();
			self.active_script.set(Some(script));
		}
	}
}

impl TabViewer for State {
	type Tab = TabType;

	fn title(&mut self, tab: &mut Self::Tab) -> eframe::egui::WidgetText {
		tab.name().into()
	}

	fn ui(&mut self, ui: &mut eframe::egui::Ui, tab: &mut Self::Tab) {
		match tab {
			TabType::ScriptInfo => self.script_info_ui(ui),
			TabType::InputDisplay => self.input_display_ui(ui),
			TabType::GameInfo => self.game_info_ui(ui),
			TabType::Log => {
				ui.add_enabled_ui(false, |ui| {
					ui.add_sized(ui.available_size(), TextEdit::multiline(&mut self.log));
				});
			}
		}
	}
}
