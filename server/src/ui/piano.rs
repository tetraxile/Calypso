use eframe::egui::{Grid, Ui};

use crate::State;

impl State {
	pub fn piano_roll_ui(&mut self, ui: &mut Ui) {
		Grid::new("piano-roll")
			.num_columns(3)
			.striped(true)
			.show(ui, |ui| {
				for i in 0..10 {
					ui.label("a");
					ui.label("b");
					ui.label("c");
					ui.end_row();
				}
			});
	}
}
