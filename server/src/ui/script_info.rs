use eframe::egui::{Button, ComboBox, Grid, TextEdit, Ui, Vec2};

use crate::{State, script_sender::ScriptMessage, server::ToServer};

impl State {
	pub fn script_info_ui(&mut self, ui: &mut Ui) {
		ui.horizontal(|ui| {
			if ui.button("Open").clicked() {
				// requires &mut frame, handled in Self::update()
				self.should_open_dialog = true;
			}

			let recent_scripts = self.config.recent_scripts.get();
			let label = self
				.config
				.recent_scripts
				.get()
				.get(0)
				.as_ref()
				.map(|path| {
					path.file_name()
						.expect("no filename")
						.to_str()
						.expect("non-utf8 filename")
				})
				.unwrap_or("Open a file to get started");

			let mut selected = None;
			ComboBox::from_id_salt("script-selector")
				.width(ui.available_width())
				.selected_text(label)
				.show_ui(ui, |ui| {
					for (index, path) in recent_scripts.iter().enumerate() {
						let alt = ui.ctx().input(|input| input.modifiers.alt);
						let path = if alt {
							path.to_str().unwrap()
						} else {
							path.file_name().unwrap().to_str().unwrap()
						};

						if ui.selectable_label(index == 0, path).clicked() {
							selected = Some(index);
						}
					}
				});

			if let Some(selected) = selected {
				self.select_script(recent_scripts[selected].clone());
			}
		});

		if let Some(script) = self.active_script.get() {
			let column_size =
				ui.available_width() / 4.0 - ui.spacing().item_spacing.x * (3.0 / 4.0);
			Grid::new("control-buttons")
				.max_col_width(column_size)
				.num_columns(4)
				.show(ui, |ui| {
					let i = Vec2 {
						x: column_size,
						y: ui.min_size().y,
					};
					if ui.add_sized(i, Button::new("Run")).clicked() {
						self.script_sender
							.blocking_send(ScriptMessage::Start)
							.unwrap()
					}
					if ui.add_sized(i, Button::new("Stop")).clicked() {
						self.script_sender
							.blocking_send(ScriptMessage::Stop { manual: true })
							.unwrap()
					}
					if ui.add_sized(i, Button::new("Pause game")).clicked() {
						self.server_sender.send(ToServer::PauseGame).unwrap()
					}
					if ui.add_sized(i, Button::new("Frame advance")).clicked() {
						self.server_sender.send(ToServer::AdvanceFrame).unwrap()
					}
				});

			Grid::new("script-info-grid").num_columns(2).show(ui, |ui| {
				ui.label("Script name");
				let mut script_name = script.path.file_name().unwrap().to_str().unwrap();
				ui.add_enabled(false, TextEdit::singleline(&mut script_name));
				ui.end_row();
				ui.label("Script author");
				let mut author = script
					.script
					.author
					.as_ref()
					.map(String::as_str)
					.unwrap_or("no author listed");
				ui.add_enabled(false, TextEdit::singleline(&mut author));
				ui.end_row();
				ui.label("Frame count");
				ui.label(&self.script_frame_text);
				ui.end_row();
				if let Some(change_stage_info) = &script.script.change_stage_info {
					ui.label("Start stage name");
					ui.label(&change_stage_info.stage_name);
					ui.end_row();
					ui.label("Start scenario");
					ui.label(change_stage_info.scenario_no.to_string());
					ui.end_row();
					ui.label("Start entrance");
					ui.label(change_stage_info.entrance_id.to_string());
					ui.end_row();
				}
			});
		}
	}
}
