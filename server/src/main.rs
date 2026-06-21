mod config;
mod script_sender;
mod server;
mod tracked_value;
pub mod ui;

use std::future::pending;

use eframe::{
	App, CreationContext, NativeOptions,
	egui::{CentralPanel, MenuBar},
};
use egui_dock::{DockArea, DockState, NodeIndex};

use crate::ui::{State, TabType};

fn main() {
	let runtime = tokio::runtime::Builder::new_current_thread()
		.enable_all()
		.build()
		.expect("failed to start tokio");

	let handle = runtime.handle().clone();
	std::thread::spawn(move || runtime.block_on(pending::<()>()));
	let _enter_guard = handle.enter();
	eframe::run_native(
		"Calypso",
		NativeOptions::default(),
		Box::new(|ctx| Ok(Box::new(Calypso::new(ctx)))),
	)
	.unwrap();
}

pub struct Calypso {
	state: State,
	dock_state: DockState<TabType>,
}

impl Calypso {
	pub fn new(ctx: &CreationContext) -> Self {
		let mut dock_state = DockState::new(vec![TabType::InputDisplay]);
		let [right, left] = dock_state.main_surface_mut().split_left(
			NodeIndex::root(),
			0.4,
			vec![TabType::ScriptInfo],
		);
		dock_state
			.main_surface_mut()
			.split_below(left, 0.5, vec![TabType::Log]);
		dock_state
			.main_surface_mut()
			.split_above(right, 0.5, vec![TabType::GameInfo]);

		Self {
			dock_state,
			state: State::new(ctx),
		}
	}
}

impl App for Calypso {
	fn update(&mut self, _ctx: &eframe::egui::Context, frame: &mut eframe::Frame) {
		self.state.update(frame);
	}

	fn ui(&mut self, ui: &mut eframe::egui::Ui, _frame: &mut eframe::Frame) {
		MenuBar::new().ui(ui, |ui| {
			ui.menu_button("Do the thing", |ui| {
				if ui.button("Hello world").clicked() {
					println!("hello world")
				}
			});
		});
		CentralPanel::no_frame().show_inside(ui, |ui| {
			DockArea::new(&mut self.dock_state)
				.show_leaf_collapse_buttons(false)
				.show_leaf_close_all_buttons(false)
				.show_inside(ui, &mut self.state);
		});
	}
}
