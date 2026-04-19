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
	println!("data length: {}", frames.len());

	for frame in frames {
		println!("frame idx: {}", frame.cur_frame);
		println!("commands:");
		for command in frame.commands {
			if let Command::Controller {
				player_num,
				buttons,
				left_stick,
				right_stick,
				left_accel,
				right_accel,
				left_gyro,
				right_gyro,
			} = command
			{
				println!("\tController");
				println!("\t\tplayer num: {player_num}");
				println!("\t\tbuttons: {:064b}", buttons.into_bits());
				println!("\t\tleft stick: {left_stick}");
				println!("\t\tright stick: {right_stick}");
				println!("\t\tleft accel: {left_accel}");
				println!("\t\tright accel: {right_accel}");
				println!("\t\tleft gyro: {left_gyro:?}");
				println!("\t\tright gyro: {right_gyro:?}");
			} else {
				bail!("balls balls balls");
			}
		}
	}

	Ok(())
}
