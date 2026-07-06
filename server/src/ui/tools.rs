use eframe::egui::Ui;
use serde::{Deserialize, Serialize};
use tokio::sync::mpsc::UnboundedSender;

use crate::{
	State,
	server::{ToServer, protocol::ToolType},
	tracked_value::TrackedValue,
};

#[derive(Serialize, Deserialize)]
pub struct Tools {
	show_ui: TrackedValue<bool>,
	always_uncollected_moons: TrackedValue<bool>,
}

impl Default for Tools {
	fn default() -> Self {
		Self {
			show_ui: true.into(),
			always_uncollected_moons: true.into(),
		}
	}
}

impl State {
	pub fn tools_ui(&mut self, ui: &mut Ui) {
		fn tracking_checkbox(ui: &mut Ui, text: &str, value: &mut TrackedValue<bool>) {
			let mut temp_value = value.read();

			if ui.checkbox(&mut temp_value, text).changed() {
				value.set(temp_value);
			}
		}
		tracking_checkbox(ui, "Show UI", &mut self.config.tools.show_ui);
		tracking_checkbox(ui, "Reactivate moons", &mut self.config.tools.always_uncollected_moons);
	}

	pub fn tools_tracking(&mut self) -> bool {
		fn track_bool_tool(
			value: &mut TrackedValue<bool>,
			tool: ToolType,
			sender: &mut UnboundedSender<ToServer>,
		) -> bool {
			let changed = value.has_changed();
			value.if_changed(|value| {
				sender
					.send(ToServer::UpdateTool(tool, [*value as u8].into()))
					.unwrap();
			});
			changed
		}
		let mut updated = false;
		updated |= track_bool_tool(
			&mut self.config.tools.show_ui,
			ToolType::ShowUi,
			&mut self.server_sender,
		);
		updated |= track_bool_tool(
			&mut self.config.tools.always_uncollected_moons,
			ToolType::AlwaysUncollectedMoons,
			&mut self.server_sender,
		);
		updated
	}
}
