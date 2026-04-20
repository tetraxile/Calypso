use num_derive::FromPrimitive;
use tas_script_formats::glam::{IVec2, Vec3};
use zerocopy::{
	FromBytes, Immutable, IntoBytes, KnownLayout,
	little_endian::{U16, U32, U64},
};

#[derive(FromBytes, IntoBytes, KnownLayout, Immutable)]
#[repr(C, align(8))]
pub struct Controller {
	pub buttons: U64,
	pub left_stick: IVec2,
	pub right_stick: IVec2,
	pub accel_left: Vec3,
	pub gyro_left: Vec3,
	pub accel_right: Vec3,
	pub gyro_right: Vec3,
}

#[derive(FromBytes, IntoBytes, KnownLayout, Immutable)]
#[repr(C)]
pub struct FramePacket {
	pub frame_idx: U32,
	_padding: u32,
	pub player_1: Controller,
	pub player_2: Controller,
	pub amiibo: U64,
}

#[derive(FromBytes, IntoBytes, KnownLayout, Immutable)]
#[repr(C)]
pub struct ServerInfoPacket {
	pub version: U16,
}

#[derive(FromBytes, IntoBytes, KnownLayout, Immutable)]
#[repr(C)]
pub struct ScriptInfoPacket {
	pub frame_count: U32,
}

#[derive(FromBytes, IntoBytes, KnownLayout, Immutable)]
#[repr(C)]
pub struct ReportPositionPacket {
	pub position: Vec3,
}

#[derive(FromPrimitive, Debug)]
pub enum PacketType {
	ServerInfo,
	ScriptInfo,
	Frame,
	Log,
	LoadSave,
	GetSave,
	ReportStageName,
	ReportPosition,
}

#[derive(Debug, FromBytes, IntoBytes, KnownLayout, Immutable)]
#[repr(C)]
pub struct PacketHeader {
	pub packet_type: u8,
	pub size: U32,
}
