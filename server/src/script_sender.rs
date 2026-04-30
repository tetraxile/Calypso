use std::{fmt::Debug, future::pending, pin::pin, sync::Arc, time::Duration};

use futures_util::{
	FutureExt,
	future::{Either, select},
};
use tas_script_formats::{ControllerType, STASButtons, Script};
use tokio::{sync::mpsc, time::Instant};
use tracing::{info, warn};
use zerocopy::FromZeros;

use crate::server::{ToServer, ToUi, protocol::Controller};

pub enum ScriptMessage {
	Script(Arc<Script>),
	Start,
	Stop { manual: bool },
	BackOff { server_index: u32 },
}

impl Debug for ScriptMessage {
	fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
		match self {
			Self::Script(_) => f.debug_tuple("Script").finish(),
			Self::Start => write!(f, "Start"),
			Self::Stop { .. } => write!(f, "Stop"),
			Self::BackOff { server_index } => f
				.debug_struct("BackOff")
				.field("server_index", server_index)
				.finish(),
		}
	}
}

pub async fn script_sender(
	mut from_ui: mpsc::Receiver<ScriptMessage>,
	to_ui: mpsc::UnboundedSender<ToUi>,
	to_server: mpsc::UnboundedSender<ToServer>,
) {
	// 62.5ms (sending at 16hz)
	let mut interval = tokio::time::interval(Duration::from_micros(62500));

	let mut current_script: Option<Arc<Script>> = None;
	let mut running = false;
	let mut stopped = false;
	let mut back_off = None;
	let mut current_frame = 0u32;
	loop {
		let sleep = running
			.then(|| {
				Either::Left(
					back_off
						.map(tokio::time::sleep_until)
						.map(Either::Left)
						.unwrap_or_else(|| Either::Right(interval.tick().map(drop))),
				)
			})
			.unwrap_or(Either::Right(pending()));
		match select(pin!(sleep), pin!(from_ui.recv())).await {
			Either::Left((_, _)) => {
				back_off = None;
				info!("sending frames {}", current_frame);

				let script = current_script
					.as_ref()
					.expect("script must be set to be running");

				for _ in 0..4 {
					let Some(frame) = script.frames.get(current_frame as usize) else {
						running = false;
						break;
					};

					let mut player_1 = Controller::new_zeroed();
					let mut player_2 = Controller::new_zeroed();
					let mut amiibo = 0u64;

					for command in &frame.commands {
						match command {
							tas_script_formats::Command::Controller(controller) => {
								let player = match controller.player_id {
									0 => &mut player_1,
									1 => &mut player_2,
									id => {
										warn!("unexpected player {id} in tas script");
										continue;
									}
								};
								let buttons = controller
									.buttons
									.unwrap_or(tas_script_formats::Buttons::new());
								player.buttons = STASButtons::from_internal(buttons);
								player.left_stick = controller.left_stick.unwrap_or_default();
								player.right_stick = controller.right_stick.unwrap_or_default();
							}
							tas_script_formats::Command::Amiibo { model_info } => {
								amiibo = *model_info
							}
							tas_script_formats::Command::Touch(_) => todo!("touch unsupported"),
							tas_script_formats::Command::Save => todo!("saves"),
							tas_script_formats::Command::Comment(_) => {}
						}
					}
					to_server
						.send(ToServer::Frame {
							frame_index: frame.idx as u32,
							server_index: current_frame,
							player_1,
							player_2,
							amiibo,
						})
						.expect("channel closed");
					current_frame += 1;
				}
			}
			Either::Right((message, _)) => {
				info!("message to script sender {message:?}");
				match message.expect("channel closed") {
					ScriptMessage::Script(script) => {
						to_server
							.send(ToServer::ScriptInfo {
								frame_count: script.frames.len() as _,
								player_count: if script.is_two_player { 2 } else { 1 },
								controller_types: [
									script.controller_types.get(0).cloned().expect("no players")
										as _,
									script
										.controller_types
										.get(1)
										.cloned()
										.unwrap_or(ControllerType::None) as _,
								],
							})
							.expect("channel closed");
						current_script = Some(script);
						running = false;
					}
					ScriptMessage::Start => {
						if current_script.is_some() {
							running = true;
							stopped = false;
							to_server
								.send(ToServer::StartScript)
								.expect("channel closed");
						}
					}
					ScriptMessage::Stop { manual } => {
						running = false;
						stopped = true;
						if !manual {
							to_ui
								.send(ToUi::ScriptPlaybackEnded)
								.expect("channel closed")
						}
						current_frame = 0;
						to_server
							.send(ToServer::StopScript)
							.expect("channel closed");
					}
					ScriptMessage::BackOff { server_index: to } => {
						if stopped || to >= current_frame {
							continue;
						}
						current_frame = to;
						running = true;
						warn!("backed off");
						back_off = Some(Instant::now() + Duration::from_millis(200));
					}
				}
			}
		}
	}
}
