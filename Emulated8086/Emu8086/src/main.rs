use std::error::Error;
use argh::FromArgs;

#[derive(FromArgs)]
/// emulation/disassembly for the 8086 processor
/// created as part of the performance aware programming series by Casey Muratori
struct Emu8086 {
    /// whether to disassemble the input file
    #[argh(switch, short = 'd')]
    disassemble: bool,

    /// file to process
    #[argh(option, short = 'f')]
    file: String,
}

fn instruction_bin_to_mnemonic(code : u8) -> String {
    match code {
        0b100010 => {String::from("mov")}
        _ => {panic!("Unimplemented code {0:b}!", code)}
    }
}

fn reg_bin_to_mnemonic(code : u8, wide : bool) -> String {
    if wide {
        match code {
            0b000 => {return String::from("ax")}
            0b001 => {return String::from("cx")}
            0b010 => {return String::from("dx")}
            0b011 => {return String::from("bx")}
            0b100 => {return String::from("sp")}
            0b101 => {return String::from("bp")}
            0b110 => {return String::from("si")}
            0b111 => {return String::from("di")}
            _ => {panic!("Unimplemented code {0:b}!", code)}
        }
    }

    match code {
        0b000 => {String::from("al")}
        0b001 => {String::from("cl")}
        0b010 => {String::from("dl")}
        0b011 => {String::from("bl")}
        0b100 => {String::from("ah")}
        0b101 => {String::from("ch")}
        0b110 => {String::from("dh")}
        0b111 => {String::from("bh")}
        _ => {panic!("Unimplemented code {0:b}!", code)}
    }
}

fn main() {
    let emu: Emu8086 = argh::from_env();
    let data = std::fs::read(emu.file).expect("Failed to open file");

    if emu.disassemble {
        let instruction_mask = 0b11111100;
        let mod_mask = 0b11000000;
        let left_reg_mask = 0b00000111;
        let right_reg_mask = 0b00111000;
        let d_mask = 0b00000010;
        let width_mask = 0b00000001;

        println!("bits 16");

        let mut i = 0;

        while i < data.len() {
            let instruction_code = (data[i] & instruction_mask) >> 2;
            let d = (data[i] & d_mask) >> 1;
            let width = data[i] & width_mask;

            i += 1;

            let regmod = (data[i] & mod_mask) >> 6;
            let left_reg = (data[i] & left_reg_mask);
            let right_reg = (data[i] & right_reg_mask) >> 3;

            println!("{0} {1}, {2} {3}", instruction_bin_to_mnemonic(instruction_code),
                     reg_bin_to_mnemonic(left_reg, width == 1),
                    reg_bin_to_mnemonic(right_reg, width == 1), d);

            //println!("{0} {1} {2}", instruction_bin_to_mnemonic(instruction_code), d, width);

            i += 1;
        }
    }
}
