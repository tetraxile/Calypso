use eframe::egui::{Grid, Ui};
use tas_script_formats::glam::Vec3;

use crate::State;

impl State {
	pub fn game_info_ui(&self, ui: &mut Ui) {
		Grid::new("game-info")
			.num_columns(2)
			.spacing([40.0, 4.0])
			.striped(true)
			.show(ui, |ui| {
				if let Some((stage_name, scenario)) = &self.stage {
					ui.label("Stage");
					self.monospace_scope(ui, |ui| ui.label(stage_name));
					ui.end_row();
					ui.label("Scenario");
					self.monospace_scope(ui, |ui| ui.label(scenario.to_string()));
					ui.end_row();
				}
				if let Some(Vec3 { x, y, z }) = &self.player_position {
					ui.label("Player Position");
					self.monospace_scope(ui, |ui| {
						ui.label(format!("{x:>10.4} {y:>10.4} {z:>10.4}"))
					});
					ui.end_row();
				}
			});
	}
}
