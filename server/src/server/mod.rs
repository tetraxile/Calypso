use std::{pin::pin, sync::Arc};

use eyre::{Context, ContextCompat, Result, bail, eyre};
use futures_util::future::{Either, select};
use num_traits::FromPrimitive;
use tas_script_formats::{ChangeStage, glam::Vec3};
use tokio::{
	io::{AsyncReadExt, AsyncWriteExt},
	net::{
		TcpListener, UdpSocket,
		tcp::{OwnedReadHalf, OwnedWriteHalf},
	},
	sync::{Mutex, mpsc},
};
use tokio_util::sync::CancellationToken;
#[allow(unused_imports)]
use tracing::{debug, error, info, warn};
use zerocopy::{FromBytes, FromZeros, IntoBytes, Unalign, little_endian::U32};

use crate::server::protocol::{Controller, FramePacket, PacketHeader, PacketType, ScriptInfo};

pub mod protocol;

pub enum ToServer {
	ScriptInfo {
		frame_count: u32,
		player_count: u8,
		controller_types: [u8; 2],
	},
	ChangeStage(ChangeStage),
	ReloadStage,
	Frame {
		frame_index: u32,
		next_frame_index: u32,
		server_index: u32,
		player_1: Controller,
		player_2: Controller,
		amiibo: u64,
	},
	GetSave {
		save_index: u8,
	},
	PauseGame,
	AdvanceFrame,
	StartScript,
	StopScript,
}

pub enum ToUi {
	Log(String),
	SaveFile(Vec<u8>),
	ClientConnected,
	ClientError(eyre::Error),
	FullFrameBuffer { server_index: u32 },
	ScriptPlaybackEnded,
	ReportStage { stage_name: String, scenario: i32 },
	ReportPosition { position: Vec3 },
}

pub async fn server_task(
	ui: mpsc::UnboundedSender<ToUi>,
	server: mpsc::UnboundedReceiver<ToServer>,
) {
	tokio::spawn(udp_task(ui.clone()));
	let tcp_listener = TcpListener::bind("0.0.0.0:8171")
		.await
		.expect("failed to start tcp server on 8171");

	let server = Arc::new(Mutex::new(server));
	let mut token: Option<CancellationToken> = None;

	loop {
		match tcp_listener.accept().await {
			Ok((stream, _addr)) => {
				if let Some(token) = token.take() {
					token.cancel();
				}

				drop(server.lock().await); // ensure the previous tasks have cleanly ended
				let (reader, writer) = stream.into_split();
				let token = token.insert(CancellationToken::new());
				tokio::spawn(handle_packets(reader, token.clone(), ui.clone()));
				tokio::spawn(handle_messages(
					writer,
					token.clone(),
					server.clone(),
					ui.clone(),
				));
				if let Err(_) = ui.send(ToUi::ClientConnected) {
					return;
				}
			}
			Err(error) => {
				error!("failed to accept client: {error}");
				continue;
			}
		};
	}
}

async fn handle_packets(
	mut stream: OwnedReadHalf,
	token: CancellationToken,
	ui: mpsc::UnboundedSender<ToUi>,
) {
	loop {
		let read_packet = pin!(read_packet(&mut stream));
		let cancelled = pin!(token.cancelled());
		match select(read_packet, cancelled).await {
			Either::Left((Ok(to_ui), _)) => {
				if let Err(_) = ui.send(to_ui) {
					return;
				}
			}
			Either::Left((Err(message), _)) => {
				let _ = ui.send(ToUi::Log(format!("{message}\n")));
				token.cancel();
				return;
			}
			Either::Right((_, _)) => return,
		}
	}
}

async fn read_packet(stream: &mut OwnedReadHalf) -> Result<ToUi> {
	let mut header = PacketHeader::new_zeroed();
	stream
		.read_exact(header.as_mut_bytes())
		.await
		.context("failed to read header")?;
	info!("header: {header:?}");

	let packet_type = PacketType::from_u8(header.packet_type).context("invalid packet type")?;
	match packet_type {
		PacketType::Log => {
			let mut text = vec![0u8; header.size.get() as usize];
			stream
				.read_exact(&mut text)
				.await
				.context("failed to read log message")?;
			Ok(ToUi::Log(
				String::from_utf8(text).context("log message was not utf-8")?,
			))
		}
		PacketType::GetSave => {
			let mut data = vec![0u8; header.size.get() as usize - 1];
			stream
				.read_exact(&mut data)
				.await
				.context("failed to read save file")?;
			Ok(ToUi::SaveFile(data))
		}
		PacketType::ReportStageName => {
			let mut name = vec![0u8; header.size.get() as usize - 4];
			let scenario = stream
				.read_i32_le()
				.await
				.context("failed to read scenario num")?;
			stream
				.read_exact(&mut name)
				.await
				.context("failed to read save file")?;
			Ok(ToUi::ReportStage {
				stage_name: String::from_utf8(name).context("stage name was not utf-8")?,
				scenario,
			})
		}
		PacketType::FullFrameBuffer => {
			let server_index = stream
				.read_u32_le()
				.await
				.context("failed to read server index")?;
			info!("server frame index: {server_index}");
			Ok(ToUi::FullFrameBuffer { server_index })
		}
		PacketType::ScriptEnded => Ok(ToUi::ScriptPlaybackEnded),
		packet_type => {
			bail!("unexpected packet type: {packet_type:?}")
		}
	}
}

async fn handle_messages(
	mut stream: OwnedWriteHalf,
	token: CancellationToken,
	server: Arc<Mutex<mpsc::UnboundedReceiver<ToServer>>>,
	ui: mpsc::UnboundedSender<ToUi>,
) {
	let mut server = server.lock().await;
	loop {
		let read_packet = pin!(server.recv());
		let cancelled = pin!(token.cancelled());
		match select(read_packet, cancelled).await {
			Either::Left((message, _)) => {
				if let Err(error) =
					handle_message(&mut stream, message.expect("channel closed")).await
				{
					if let Err(_) = ui.send(ToUi::ClientError(error)) {
						return;
					}
				}
			}
			Either::Right((_, _)) => return,
		}
	}
}

async fn handle_message(client: &mut OwnedWriteHalf, message: ToServer) -> Result<()> {
	match message {
		ToServer::ScriptInfo {
			frame_count,
			player_count,
			controller_types,
		} => {
			client
				.write_all(
					PacketHeader {
						packet_type: PacketType::ScriptInfo as _,
						size: U32::new(size_of::<ScriptInfo>() as u32),
					}
					.as_bytes(),
				)
				.await
				.context("failed to write script info packet header")?;
			let mut script_info = ScriptInfo::new_zeroed();
			script_info.frame_count = U32::new(frame_count);
			script_info.player_count = player_count;
			script_info.controller_types = controller_types;
			client
				.write_all(script_info.as_bytes())
				.await
				.context("failed to write script info")?;
		}
		ToServer::ChangeStage(ChangeStage {
			stage_name,
			entrance_id,
			scenario_no,
			sub_scenario,
			is_return,
		}) => {
			client
				.write_all(
					PacketHeader {
						packet_type: PacketType::ChangeStage as _,
						size: U32::new(
							(4 + 1 + 1 + 2 + 2 + stage_name.len() + 1 + entrance_id.len() + 1) as _,
						),
					}
					.as_bytes(),
				)
				.await
				.context("failed to write script info packet header")?;
			client
				.write_i32_le(scenario_no)
				.await
				.context("failed to write scenario no")?;
			client
				.write_u8(sub_scenario)
				.await
				.context("failed to write sub scenario")?;
			client
				.write_u8(is_return as _)
				.await
				.context("failed to write is return")?;
			client
				.write_u16_le(stage_name.len() as u16 + 1)
				.await
				.context("failed to write stage name length")?;
			client
				.write_u16_le(entrance_id.len() as u16 + 1)
				.await
				.context("failed to write entrance id length")?;
			client
				.write_all(stage_name.as_bytes())
				.await
				.context("failed to write stage name")?;
			client
				.write_u8(0)
				.await
				.context("failed to write stage name null terminator")?;
			client
				.write_all(entrance_id.as_bytes())
				.await
				.context("failed to write entrance id")?;
			client
				.write_u8(0)
				.await
				.context("failed to write entrance id null terminator")?;
		}
		ToServer::ReloadStage => client
			.write_all(
				PacketHeader {
					packet_type: PacketType::ReloadStage as _,
					size: 0.into(),
				}
				.as_bytes(),
			)
			.await
			.context("failed to write pause packet")?,
		ToServer::Frame {
			frame_index,
			next_frame_index,
			server_index,
			player_1,
			player_2,
			amiibo,
		} => {
			let packet = FramePacket {
				frame_index: frame_index.into(),
				next_frame_index: next_frame_index.into(),
				server_index: server_index.into(),
				player_1: Unalign::new(player_1),
				player_2: Unalign::new(player_2),
				amiibo: amiibo.into(),
			};

			client
				.write_all(
					PacketHeader {
						packet_type: PacketType::Frame as _,
						size: U32::new(size_of::<FramePacket>() as u32),
					}
					.as_bytes(),
				)
				.await
				.context("failed to write script info packet header")?;
			client
				.write_all(packet.as_bytes())
				.await
				.context("failed to write frame count")?;
		}
		ToServer::GetSave { save_index: _ } => {
			warn!("not sending get save");
		}
		ToServer::PauseGame => client
			.write_all(
				PacketHeader {
					packet_type: PacketType::PauseGame as _,
					size: 0.into(),
				}
				.as_bytes(),
			)
			.await
			.context("failed to write pause packet")?,
		ToServer::AdvanceFrame => client
			.write_all(
				PacketHeader {
					packet_type: PacketType::AdvanceFrame as _,
					size: 0.into(),
				}
				.as_bytes(),
			)
			.await
			.context("failed to write advance packet")?,
		ToServer::StartScript => client
			.write_all(
				PacketHeader {
					packet_type: PacketType::StartScript as _,
					size: 0.into(),
				}
				.as_bytes(),
			)
			.await
			.context("failed to write start packet")?,
		ToServer::StopScript => client
			.write_all(
				PacketHeader {
					packet_type: PacketType::StopScript as _,
					size: 0.into(),
				}
				.as_bytes(),
			)
			.await
			.context("failed to write stop packet")?,
	}

	client.flush().await.context("failed to flush")?;

	Ok(())
}

async fn udp_task(ui: mpsc::UnboundedSender<ToUi>) {
	let udp = UdpSocket::bind("0.0.0.0:8171")
		.await
		.expect("failed to bind udp server on 8171");

	info!(
		"listening on udp {}",
		udp.local_addr()
			.expect("failed to get local listening addr")
	);

	let mut buffer = [0; 800];
	loop {
		let (size, addr) = udp.recv_from(&mut buffer).await.unwrap();
		let buffer = &buffer[..size];

		match handle_udp_message(buffer).await {
			Ok(UdpMessage::Ui(to_ui)) => {
				if let Err(_) = ui.send(to_ui) {
					return;
				}
			}
			Ok(UdpMessage::DiscoveryReply) => {
				if 0 == udp
					.send_to(b"hi", addr)
					.await
					.expect("failed to send anything")
				{
					panic!("sending nothing")
				}
			}
			Err(error) => {
				error!("failed to read udp packet: {error}");
			}
		}
	}
}

enum UdpMessage {
	Ui(ToUi),
	DiscoveryReply,
}

async fn handle_udp_message(data: &[u8]) -> Result<UdpMessage> {
	// info!("udp packet: {data:02X?}");
	let (header, data) =
		PacketHeader::ref_from_prefix(data).map_err(|_| eyre!("failed to read packet header"))?;

	let packet_type = PacketType::from_u8(header.packet_type).context("invalid packet type")?;
	match packet_type {
		PacketType::ReportPosition => {
			let (position, _) = Vec3::read_from_prefix(data)
				.map_err(|_| eyre!("failed to read position report"))?;
			Ok(UdpMessage::Ui(ToUi::ReportPosition { position }))
		}
		PacketType::UDPDiscovery => Ok(UdpMessage::DiscoveryReply),
		packet_type => bail!("unexpected packet type: {packet_type:?}"),
	}
}
