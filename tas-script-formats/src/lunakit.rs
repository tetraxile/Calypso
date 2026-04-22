use crate::{Script, internal};
use bitfield_struct::bitfield;
use eyre::{Result, ensure, eyre};
use glam::{Mat3, Vec2, Vec3};
use zerocopy::{Immutable, KnownLayout, TryFromBytes, little_endian::*};

#[repr(C)]
#[derive(TryFromBytes, KnownLayout, Immutable)]
struct Gyro {
	direction: Mat3,
	angular_v: Vec3,
}

#[bitfield(u32)]
#[derive(TryFromBytes, KnownLayout, Immutable)]
struct Buttons {
	a: bool,
	b: bool,
	zl: bool,
	x: bool,
	y: bool,
	zr: bool,
	right_stick: bool,
	left_stick: bool,
	home: bool,
	minus: bool,
	plus: bool,
	start: bool,
	select: bool,
	l: bool,
	r: bool,
	touch: bool,
	up: bool,
	down: bool,
	left: bool,
	right: bool,
	left_stick_up: bool,
	left_stick_down: bool,
	left_stick_left: bool,
	left_stick_right: bool,
	right_stick_up: bool,
	right_stick_down: bool,
	right_stick_left: bool,
	right_stick_right: bool,
	#[bits(4)]
	__: u32,
}

impl Buttons {
	fn to_internal(&self) -> internal::Buttons {
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
			.with_home(self.home())
	}
}

#[repr(C)]
#[derive(TryFromBytes, KnownLayout, Immutable)]
struct Frame {
	step: U32,
	is_second_player: bool,
	buttons: Buttons,
	left_stick: Vec2,
	right_stick: Vec2,
	left_accel: Vec3,
	right_accel: Vec3,
	left_gyro: Gyro,
	right_gyro: Gyro,
}

impl Frame {
	fn to_internal(&self) -> Vec<internal::Command> {
		vec![internal::Command::Controller(internal::Controller {
			player_id: if self.is_second_player { 1 } else { 0 },
			buttons: Some(self.buttons.to_internal()),
			left_stick: Some(internal::convert_joystick(self.left_stick)),
			right_stick: Some(internal::convert_joystick(self.right_stick)),
			left_accel: Some(self.left_accel),
			right_accel: Some(self.right_accel),
			left_gyro: Some(internal::Gyro {
				direction: Mat3::IDENTITY,
				angular_v: Vec3::ZERO,
			}),
			right_gyro: Some(internal::Gyro {
				direction: Mat3::IDENTITY,
				angular_v: Vec3::ZERO,
			}),
		})]
	}
}

#[repr(C)]
#[derive(TryFromBytes, KnownLayout, Immutable, Debug)]
struct Header {
	signature: [u8; 4],
	frame_count: U32,
	is_two_player: bool,
	scenario_no: I32,
	change_stage_name: [u8; 0x80],
	change_stage_id: [u8; 0x80],
	start_position: Vec3,
}

pub fn parse_lunakit(data: &[u8]) -> Result<Script> {
	let (header, rest) =
		Header::try_ref_from_prefix(data).map_err(|_| eyre!("failed to parse script header"))?;

	let frames = <[Frame]>::try_ref_from_bytes_with_elems(rest, header.frame_count.get() as usize)
		.map_err(|_| eyre!("failed to parse script frames"))?;

	println!("frame count: {}", frames.len());

	let frames = if header.is_two_player {
		frames
			.chunks_exact(2)
			.map(|chunk| {
				let part_1 = &chunk[0];
				let part_2 = &chunk[1];
				ensure!(
					part_1.step.get() == part_2.step.get(),
					"invalid two-player script"
				);
				let mut commands = vec![];
				commands.extend(part_1.to_internal());
				commands.extend(part_2.to_internal());

				Ok(internal::Frame {
					idx: part_1.step.get() as usize,
					commands,
				})
			})
			.collect::<Result<Vec<_>>>()?
	} else {
		frames
			.iter()
			.map(|f| internal::Frame {
				idx: f.step.get() as usize,
				commands: f.to_internal(),
			})
			.collect::<Vec<_>>()
	};

	Ok(Script {
		author: None,
		seconds_spent_editing: None,
		is_two_player: header.is_two_player,
		frames,
	})
}
