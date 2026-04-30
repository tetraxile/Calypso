use num_derive::FromPrimitive;
use tas_script_formats::{
	STASButtons,
	glam::{IVec2, Vec3},
};
use zerocopy::{
	FromBytes, Immutable, IntoBytes, KnownLayout,
	little_endian::{U32, U64},
};

#[derive(FromBytes, IntoBytes, KnownLayout, Immutable)]
#[repr(C, align(4))]
pub struct Controller {
	pub buttons: STASButtons,
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
	pub frame_index: U32,
	pub server_index: U32,
	pub player_1: Controller,
	pub player_2: Controller,
	pub amiibo: U64,
}

// #[derive(FromBytes, IntoBytes, KnownLayout, Immutable)]
// #[repr(C)]
// pub struct ServerInfoPacket {
// 	pub version: U16,
// }

#[derive(FromBytes, IntoBytes, KnownLayout, Immutable)]
#[repr(C, align(4))]
pub struct ScriptInfo {
	pub frame_count: U32,
	pub player_count: u8,
	pub controller_types: [u8; 2],
	_padding: u8,
}

#[derive(FromPrimitive, Debug)]
pub enum PacketType {
	ServerInfo = 0,
	ScriptInfo = 1,
	Frame = 2,
	Log = 3,
	LoadSave = 4,
	GetSave = 5,
	ReportStageName = 6,
	ReportPosition = 7,
	PauseGame = 8,
	AdvanceFrame = 9,
	UDPDiscovery = 10,
	FullFrameBuffer = 11,
	StartScript = 12,
	StopScript = 13,
	ScriptEnded = 14,
}

#[derive(Debug, FromBytes, IntoBytes, KnownLayout, Immutable)]
#[repr(C)]
pub struct PacketHeader {
	pub packet_type: u8,
	pub size: U32,
}
