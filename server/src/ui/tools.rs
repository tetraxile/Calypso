use std::borrow::Cow;

use eframe::egui::{DragValue, Grid, Response, Slider, Ui};
use serde::{Deserialize, Serialize};
use tas_script_formats::ChangeStage;
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
	change_stage_info: ChangeStageInfo,
}

#[derive(Default, Serialize, Deserialize)]
pub struct ChangeStageInfo {
	pub stage_name: TrackedValue<String>,
	pub entrance_id: TrackedValue<String>,
	pub scenario_no: TrackedValue<i32>,
	pub use_default_scenario: TrackedValue<bool>,
	pub sub_scenario: TrackedValue<u8>,
	pub is_return: TrackedValue<bool>,
}

impl Default for Tools {
	fn default() -> Self {
		Self {
			show_ui: true.into(),
			always_uncollected_moons: true.into(),
			change_stage_info: ChangeStageInfo {
				entrance_id: "start".to_owned().into(),
				scenario_no: (1i32).into(),
				..Default::default()
			},
		}
	}
}

impl State {
	pub fn tools_ui(&mut self, ui: &mut Ui) {
		fn tracking<T: Clone>(
			ui: &mut Ui,
			value: &mut TrackedValue<T>,
			widget: impl FnOnce(&mut Ui, &mut T) -> Response,
		) {
			let mut temp_value = value.read();

			let response = widget(ui, &mut temp_value);
			if response.changed() {
				value.set(temp_value);
			}
		}
		fn tracking_string(
			ui: &mut Ui,
			value: &mut TrackedValue<String>,
			widget: impl FnOnce(&mut Ui, &mut Cow<str>) -> Response,
		) {
			let mut temp_value = Cow::from(value.get());

			let response = widget(ui, &mut temp_value);
			if response.changed() {
				value.set(Cow::into_owned(temp_value));
			}
		}
		let tools = &mut self.config.tools;
		let tracking_checkbox = |ui: &mut Ui, text: &str, value: &mut TrackedValue<bool>| {
			tracking(ui, value, move |ui, value| ui.checkbox(value, text));
		};
		tracking_checkbox(ui, "Show UI", &mut tools.show_ui);
		tracking_checkbox(ui, "Reactivate moons", &mut tools.always_uncollected_moons);
		Grid::new("change-stage-info").show(ui, |ui| {
			ui.label("Stage name");
			tracking_string(ui, &mut tools.change_stage_info.stage_name, |ui, value| {
				ui.text_edit_singleline(value)
			});
			ui.end_row();
			ui.label("Entrance ID");
			tracking_string(ui, &mut tools.change_stage_info.entrance_id, |ui, value| {
				ui.text_edit_singleline(value)
			});
			ui.end_row();
			ui.label("Scenario no");
			tracking(ui, &mut tools.change_stage_info.scenario_no, |ui, value| {
				ui.add_enabled(
					!tools.change_stage_info.use_default_scenario.read(),
					Slider::new(value, 1..=15).integer(),
				)
			});
			tracking_checkbox(ui, "Default scenario", &mut tools.change_stage_info.use_default_scenario);
			ui.end_row();
			ui.label("Sub-scenario");
			tracking(
				ui,
				&mut tools.change_stage_info.sub_scenario,
				|ui, value| ui.add(DragValue::new(value).range(0..=15)),
			);
			ui.end_row();
			ui.label("Go to return entrance");
			tracking_checkbox(ui, "Return", &mut tools.change_stage_info.is_return);
			if ui.button("Change stage").clicked() {
				self.server_sender
					.send(ToServer::ChangeStage(ChangeStage {
						stage_name: tools.change_stage_info.stage_name.read(),
						entrance_id: tools.change_stage_info.entrance_id.read(),
						scenario_no: tools.change_stage_info.scenario_no.read(),
						sub_scenario: tools.change_stage_info.sub_scenario.read(),
						is_return: tools.change_stage_info.is_return.read(),
					}))
					.expect("game closed");
			}
		});
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
		fn track_unsynced<T>(value: &mut TrackedValue<T>) -> bool {
			let changed = value.has_changed();
			value.clear_changed();
			changed
		}
		let tools = &mut self.config.tools;
		let mut updated = false;
		updated |= track_bool_tool(
			&mut tools.show_ui,
			ToolType::ShowUi,
			&mut self.server_sender,
		);
		updated |= track_bool_tool(
			&mut tools.always_uncollected_moons,
			ToolType::AlwaysUncollectedMoons,
			&mut self.server_sender,
		);
		updated |= track_unsynced(&mut tools.change_stage_info.stage_name);
		updated |= track_unsynced(&mut tools.change_stage_info.entrance_id);
		updated |= track_unsynced(&mut tools.change_stage_info.scenario_no);
		updated |= track_unsynced(&mut tools.change_stage_info.sub_scenario);
		updated |= track_unsynced(&mut tools.change_stage_info.is_return);
		updated
	}
}
