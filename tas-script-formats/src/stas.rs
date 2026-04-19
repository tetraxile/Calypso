use crate::{Script, internal};
use eyre::{Result, ensure, eyre};
use zerocopy::{
	FromBytes, Immutable, IntoBytes, KnownLayout,
	little_endian::{U16, U64},
};

#[derive(IntoBytes, FromBytes, KnownLayout, Immutable)]
#[repr(C)]
struct StasHeader {
	magic: [u8; 4],
	format_version: U16,
	addon_version: U16,
	editor_extras: U16,
	title_id: U64,
}

#[derive(IntoBytes, FromBytes, KnownLayout)]
struct ScriptHeader {}

pub fn parse_stas(data: &[u8]) -> Result<Script> {
	let (header, data) =
		StasHeader::ref_from_prefix(data).map_err(|_| eyre!("failed to read file header"))?;

	ensure!(header.magic == *b"STAS", "not a Switch TAS (STAS) file");
	ensure!(
		header.format_version <= 0x0001,
		"only v0 and v1 Switch TAS files are supported"
	);
	ensure!(
		header.addon_version.get() == 0x0000,
		"unsupported addon version"
	);
	ensure!(
		header.title_id.get() == 0x0100000000010000,
		"not a tas made for Odyssey"
	);
	
	todo!()
}

// pub fn save_stas(script: Script) -> Vec<u8> {}
