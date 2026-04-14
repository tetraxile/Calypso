use eyre::*;
use std::{env, fs};
use tas_script_formats::*;

fn main() -> Result<()> {
    let args: Vec<String> = env::args().collect();

    ensure!(args.len() == 3, "usage: example/balls <format> <input file>");

    let format = &args[1];
    let in_data = fs::read(&args[2]).expect("could not read file");

    let frames = match format.as_str() {
        "lunakit" => parse_lunakit(&in_data)?,
        "stas" => parse_stas(&in_data)?,
        "tsvtas" => parse_tsvtas(&in_data)?,
        "nxtas" => parse_nxtas(&in_data)?,
        _ => bail!("unsupported script format {format:?}"),
    };

    println!("format: {format}");
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
