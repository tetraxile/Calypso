mod internal;
mod lunakit;
mod nxtas;
mod stas;
mod tsvtas;
use eyre::Result;
pub use glam;
pub use internal::*;
pub use lunakit::parse_lunakit;
pub use nxtas::parse_nxtas;
pub use stas::parse_stas;
pub use tsvtas::parse_tsvtas;

#[derive(Debug)]
pub enum GuessedFormat {
	SwitchTAS,
	Lunakit,
	NxTas,
	Unknown,
}

pub fn guess_format(data: &[u8]) -> GuessedFormat {
	if data.starts_with(b"STAS") {
		return GuessedFormat::SwitchTAS;
	}

	if data.starts_with(b"BOOB") {
		return GuessedFormat::Lunakit;
	}

	let line_end = data
		.iter()
		.position(|x| x.is_ascii_whitespace())
		.unwrap_or(data.len());
	let Ok(line) = str::from_utf8(&data[..line_end]) else {
		return GuessedFormat::Unknown;
	};

	match line.parse::<u32>() {
		Ok(_) => GuessedFormat::NxTas,
		Err(_) => GuessedFormat::Unknown,
	}
}

pub fn parse(data: &[u8]) -> Result<Script> {
	match guess_format(data) {
			GuessedFormat::SwitchTAS => parse_stas(data),
			GuessedFormat::Lunakit => parse_lunakit(data),
			GuessedFormat::NxTas => parse_nxtas(data),
			GuessedFormat::Unknown => todo!(),
	}
}
