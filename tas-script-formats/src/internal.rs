use bitfield_struct::bitfield;
use glam::{IVec2, Mat3, Vec2, Vec3};
use num_derive::FromPrimitive;

#[derive(Debug)]
pub struct Script {
	pub author: Option<String>,
	pub seconds_spent_editing: Option<u32>,
	pub is_two_player: bool,
	pub frames: Vec<Frame>,
}

#[derive(Debug)]
pub struct Frame {
	pub idx: usize,
	pub commands: Vec<Command>,
}

#[bitfield(u64)]
pub struct Buttons {
	pub a: bool,
	pub b: bool,
	pub x: bool,
	pub y: bool,
	pub zl: bool,
	pub zr: bool,
	pub l: bool,
	pub r: bool,
	pub minus: bool,
	pub plus: bool,
	pub up: bool,
	pub down: bool,
	pub left: bool,
	pub right: bool,
	pub right_stick: bool,
	pub left_stick: bool,
	pub home: bool,
	#[bits(47)]
	__: u64,
}

#[derive(Debug, FromPrimitive)]
pub enum TouchAttribute {
	Start,
	End,
}

#[derive(Debug)]
pub struct TouchEntry {
	pub attribute: TouchAttribute,
	pub finger_id: u32,
	pub position: IVec2,
}

#[derive(Debug)]
pub struct Gyro {
	pub direction: Mat3,
	pub angular_v: Vec3,
}

#[derive(Debug)]
pub struct Controller {
	pub player_id: u8,
	pub buttons: Option<Buttons>,
	pub left_stick: Option<IVec2>,
	pub right_stick: Option<IVec2>,
	pub left_accel: Option<Vec3>,
	pub right_accel: Option<Vec3>,
	pub left_gyro: Option<Gyro>,
	pub right_gyro: Option<Gyro>,
}

impl Controller {
	pub fn new(player_id: u8) -> Controller {
		Controller {
			player_id,
			buttons: None,
			left_stick: None,
			right_stick: None,
			left_accel: None,
			right_accel: None,
			left_gyro: None,
			right_gyro: None,
		}
	}
}

#[derive(Debug)]
pub enum Command {
	Controller(Controller),
	Amiibo { model_info: u64 },
	Touch(Vec<TouchEntry>),
	Save,
	Comment(String),
}

pub fn convert_joystick(input: Vec2) -> IVec2 {
	IVec2::new((input.x * 32767.0) as i32, (input.y * 32767.0) as i32)
}
