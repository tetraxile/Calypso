use eyre::*;
use std::{env, fs};
use tas_script_formats::*;

fn main() -> Result<()> {
	let args: Vec<String> = env::args().collect();

	ensure!(args.len() == 2, "usage: example/balls <input file>");

	let in_data = fs::read(&args[1]).expect("could not read file");
	println!("format: {:?}", guess_format(&in_data));

	let script = parse(&in_data).expect("failed to read file");

	let frames = script.frames;
	println!("frame count: {}", frames.len());

	Ok(())
}
