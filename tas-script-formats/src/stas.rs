use crate::util::align_stream;
use crate::{ControllerType, Script, internal};
use bitfield_struct::bitfield;
use byteordered::byteorder::LittleEndian;
use byteordered::{ByteOrdered, StaticEndianness};
use eyre::{ContextCompat, Result, ensure, eyre};
use glam::{IVec2, Vec3};
use num_derive::FromPrimitive;
use num_traits::FromPrimitive;
use std::collections::HashMap;
use std::io::{Cursor, Read, Seek};
use zerocopy::{FromBytes, Immutable, IntoBytes, KnownLayout};

type BinaryReader<'a> = ByteOrdered<Cursor<&'a [u8]>, StaticEndianness<LittleEndian>>;

#[bitfield(u64)]
#[derive(FromBytes, IntoBytes, KnownLayout, Immutable)]
pub struct Buttons {
	a: bool,
	b: bool,
	x: bool,
	y: bool,
	left_stick: bool,
	right_stick: bool,
	l: bool,
	r: bool,
	zl: bool,
	zr: bool,
	plus: bool,
	minus: bool,
	left: bool,
	up: bool,
	right: bool,
	down: bool,
	#[bits(48)]
	__: u64,
}

impl Buttons {
	pub fn to_internal(&self) -> internal::Buttons {
		internal::Buttons::new()
			.with_a(self.a())
			.with_b(self.b())
			.with_x(self.x())
			.with_y(self.y())
			.with_zl(self.zl())
			.with_zr(self.zr())
			.with_l(self.l())
			.with_r(self.r())
			.with_minus(self.minus())
			.with_plus(self.plus())
			.with_up(self.up())
			.with_down(self.down())
			.with_left(self.left())
			.with_right(self.right())
			.with_right_stick(self.right_stick())
			.with_left_stick(self.left_stick())
	}

	pub fn from_internal(buttons: internal::Buttons) -> Self {
		Self::new()
			.with_a(buttons.a())
			.with_b(buttons.b())
			.with_x(buttons.x())
			.with_y(buttons.y())
			.with_zl(buttons.zl())
			.with_zr(buttons.zr())
			.with_l(buttons.l())
			.with_r(buttons.r())
			.with_minus(buttons.minus())
			.with_plus(buttons.plus())
			.with_up(buttons.up())
			.with_down(buttons.down())
			.with_left(buttons.left())
			.with_right(buttons.right())
			.with_right_stick(buttons.right_stick())
			.with_left_stick(buttons.left_stick())
	}
}

#[derive(Debug, FromPrimitive)]
enum CommandType {
	Frame,
	Controller,
	Motion,
	Amiibo,
	Touch,
	Comment = 32768,
}

#[derive(Debug, FromPrimitive)]
enum ControllerID {
	Left,
	Right,
	Both,
}

#[derive(Debug)]
enum Command {
	Frame(u32),
	Controller {
		player_id: u8,
		buttons: Buttons,
		left_stick: IVec2,
		right_stick: IVec2,
	},
	Motion {
		player_id: u8,
		controller_id: ControllerID,
		accel: Vec3,
		gyro: Vec3,
	},
	Amiibo {
		model_info: u64,
	},
	Touch(Vec<internal::TouchEntry>),
	Comment(String),
}

fn parse_command(reader: &mut BinaryReader, format_version: u16) -> Result<Command> {
	// println!(
	// 	"offset: {:#x}",
	// 	reader.stream_position()?
	// );

	let command_type = {
		let value = reader.read_u16()?;
		CommandType::from_u16(value).ok_or(eyre!("invalid command type (got: {value})"))
	}?;

	let command_size = match format_version {
		0 => reader.read_u16()? as usize,
		1 => {
			let meow = reader.read_u64()? & 0xffff_ffff_ffff;
			reader.seek_relative(-2)?;
			meow as usize
		}
		_ => unreachable!(),
	};

	// println!("\ttype: {command_type:?}");
	// println!("\tsize: {command_size:#x}");

	match command_type {
		CommandType::Frame => {
			ensure!(
				command_size == 4,
				"FRAME command size must be 0x4 bytes (got: {command_size:#x})"
			);

			let frame_idx = reader.read_u32()?;
			Ok(Command::Frame(frame_idx))
		}
		CommandType::Controller => {
			let player_id = reader.read_u8()?;
			let buttons: Buttons;

			match format_version {
				0 => {
					ensure!(
						command_size == 0x1c,
						"CONTROLLER command size must be 0x1c bytes (got: {command_size:#x})"
					);

					align_stream(reader, 4)?;
					buttons = Buttons::from_bits(reader.read_u64()?);
				}
				1 => {
					ensure!(
						command_size == 0x18,
						"CONTROLLER command size must be 0x18 bytes (got: {command_size:#x})"
					);

					buttons = Buttons::from_bits(reader.read_u64()? & 0xff_ffff_ffff_ffff);
					reader.seek_relative(-1)?;
				}
				_ => unreachable!(),
			}

			let left_stick = IVec2::new(reader.read_i32()?, reader.read_i32()?);
			let right_stick = IVec2::new(reader.read_i32()?, reader.read_i32()?);

			Ok(Command::Controller {
				player_id,
				buttons,
				left_stick,
				right_stick,
			})
		}
		CommandType::Motion => {
			ensure!(
				command_size == 0x1c,
				"MOTION command size must be 0x1c bytes (got: {command_size:#x})"
			);

			let player_id = reader.read_u8()?;
			let controller_id = {
				let value = reader.read_u8()?;
				ControllerID::from_u8(value).ok_or(eyre!("invalid controller ID (got: {value})"))
			}?;

			align_stream(reader, 4)?;
			let accel = Vec3::new(reader.read_f32()?, reader.read_f32()?, reader.read_f32()?);
			let gyro = Vec3::new(reader.read_f32()?, reader.read_f32()?, reader.read_f32()?);

			Ok(Command::Motion {
				player_id,
				controller_id,
				accel,
				gyro,
			})
		}
		CommandType::Amiibo => {
			ensure!(
				command_size == 0x8,
				"AMIIBO command size must be 0x8 bytes (got: {command_size:#x})"
			);

			Ok(Command::Amiibo {
				model_info: reader.read_u64()?,
			})
		}
		CommandType::Touch => {
			let entry_count = reader.read_u32()?;

			let entries = (0..entry_count)
				.map(|_| {
					let attribute = reader.read_u8()?;
					align_stream(reader, 4)?;
					let finger_id = reader.read_u32()?;
					let position = IVec2::new(reader.read_i32()?, reader.read_i32()?);

					Ok(internal::TouchEntry {
						attribute: internal::TouchAttribute::from_u8(attribute)
							.ok_or(eyre!("invalid touch attribute (got: {attribute})"))?,
						finger_id,
						position,
					})
				})
				.collect::<Result<Vec<_>>>()?;

			Ok(Command::Touch(entries))
		}
		CommandType::Comment => {
			let mut comment = vec![0u8; command_size];
			reader.read_exact(&mut comment)?;
			align_stream(reader, 4)?;

			Ok(Command::Comment(String::from_utf8(comment)?))
		}
	}
}

#[allow(unused_assignments)]
pub fn parse_stas(data: &[u8]) -> Result<Script> {
	let mut reader = ByteOrdered::le(Cursor::new(data));

	ensure!(data.starts_with(b"STAS"), "not a Switch TAS (STAS) file");

	reader.seek_relative(4)?;
	let format_version = reader.read_u16()?;
	let addon_version = reader.read_u16()?;
	let _editor_extras = reader.read_u16()?;
	reader.seek_relative(2)?;
	let title_id = reader.read_u64()?;

	ensure!(
		title_id == 0x0100000000010000,
		"not a tas made for Odyssey (title ID: {title_id:016x})"
	);
	ensure!(
		addon_version == 0x0000,
		"unsupported addon version: {addon_version}"
	);
	ensure!(
		format_version <= 1,
		"only v0 and v1 Switch TAS files are supported (got {format_version})"
	);

	let stas_command_count = reader.read_u32()? as usize;
	let frame_count = match format_version {
		0 => None,
		1 => Some(reader.read_u32()? as usize),
		_ => unreachable!(),
	};
	let seconds_edited = reader.read_u32()?;

	let player_count: usize;
	let mut author_name: Option<String> = None;
	let mut controller_types: Vec<u8>;

	match format_version {
		0 => {
			player_count = reader.read_u8()? as usize;
			align_stream(&mut reader, 4)?;

			controller_types = vec![0u8; 4];
			reader.read_exact(&mut controller_types)?;
			controller_types.truncate(player_count);

			let author_name_len = reader.read_u16()? as usize;
			if author_name_len != 0 {
				let mut author_name_bytes = vec![0u8; author_name_len + 1];
				reader.read_exact(&mut author_name_bytes)?;
				author_name = Some(String::from_utf8(author_name_bytes)?);
			}

			align_stream(&mut reader, 4)?;
		}
		1 => {
			controller_types = vec![0u8; 8];
			reader.read_exact(&mut controller_types)?;
			player_count = controller_types.iter().position(|t| *t == 0).unwrap_or(8);
			controller_types.truncate(player_count);

			let author_name_len = reader.read_u16()? as usize;
			if author_name_len != 0 {
				let mut author_name_bytes = vec![0u8; author_name_len + 1];
				reader.read_exact(&mut author_name_bytes)?;
				author_name = Some(String::from_utf8(author_name_bytes)?);
			}

			align_stream(&mut reader, 4)?;

			let game_header_size = reader.read_u64()? as usize;
			let mut game_header = vec![0u8; game_header_size];
			reader.read_exact(&mut game_header)?;

			align_stream(&mut reader, 4)?;
		}
		_ => unreachable!(),
	}

	let controller_types = controller_types
		.into_iter()
		.map(ControllerType::from_u8)
		.collect::<Option<Vec<_>>>()
		.context("a controller type was invalid")?;

	ensure!(
		player_count == 1 || player_count == 2,
		"player count must be 1 or 2 (got: {player_count})"
	);

	let mut frames: Vec<internal::Frame> = if let Some(frame_count) = frame_count {
		Vec::with_capacity(frame_count)
	} else {
		vec![]
	};

	let mut last_frame_idx = 0;
	let mut read_first_frame = false;
	let mut cur_stas_commands = vec![];

	for _ in 0..(usize::min(stas_command_count, 200)) {
		let command = parse_command(&mut reader, format_version)?;

		match command {
			Command::Frame(frame_idx) => {
				ensure!(
					!read_first_frame || frame_idx > last_frame_idx,
					"frames must be in increasing order"
				);

				if !read_first_frame {
					read_first_frame = true;
					// pre-frame commands

					cur_stas_commands.clear();
					continue;
				}

				let mut controllers: HashMap<u8, internal::Controller> = HashMap::with_capacity(2);
				let mut frame_commands: Vec<internal::Command> = vec![];

				for command in cur_stas_commands.drain(..) {
					match command {
						Command::Frame(_) => unreachable!(),
						Command::Controller {
							player_id,
							buttons,
							left_stick,
							right_stick,
						} => {
							controllers
								.entry(player_id)
								.and_modify(|c| {
									c.buttons = Some(buttons.to_internal());
									c.left_stick = Some(left_stick);
									c.right_stick = Some(right_stick);
								})
								.or_insert(internal::Controller::new(player_id));
						}
						Command::Motion {
							player_id,
							controller_id,
							accel,
							gyro: _,
						} => {
							controllers
								.entry(player_id)
								.and_modify(|c| match controller_id {
									ControllerID::Left => {
										c.left_accel = Some(accel);
										c.left_gyro = None; // TODO
									}
									ControllerID::Right => {
										c.right_accel = Some(accel);
										c.right_gyro = None; // TODO
									}
									ControllerID::Both => {
										c.left_accel = Some(accel);
										c.left_gyro = None; // TODO
										c.right_accel = Some(accel);
										c.right_gyro = None; // TODO
									}
								})
								.or_insert(internal::Controller::new(player_id));
						}
						Command::Amiibo { model_info } => {
							frame_commands.push(internal::Command::Amiibo { model_info });
						}
						Command::Touch(items) => {
							frame_commands.push(internal::Command::Touch(items));
						}
						Command::Comment(message) => {
							frame_commands.push(internal::Command::Comment(message));
						}
					}
				}

				frame_commands.extend(
					controllers
						.into_values()
						.map(|c| internal::Command::Controller(c)),
				);

				frames.push(internal::Frame {
					idx: last_frame_idx as usize,
					commands: frame_commands,
				});
				last_frame_idx = frame_idx;
			}
			command => {
				cur_stas_commands.push(command);
			}
		};
	}

	// if let Some(frame) = cur_frame {
	// 	frames.push(frame);
	// }

	Ok(Script {
		author: author_name,
		seconds_spent_editing: Some(seconds_edited),
		is_two_player: player_count == 2,
		controller_types,
		frames,
	})
}

// pub fn save_stas(script: Script) -> Vec<u8> {}
