use std::mem::swap;
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
        0b100010 | 0b110001 | 0b1011 | 0b101000 | 0b100011 => {String::from("mov")}
        0b000000 | 0b100000 | 0b000001 => {String::from("add")}
        0b001010 | 0b001011 => {String::from("sub")}
        0b001110 | 0b100000 | 0b001111 => {String::from("cmp")}
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

fn bin_to_address_calc(code : u8) -> String {
    match code {
        0b000 => {String::from("[bx + si]")}
        0b001 => {String::from("[bx + di]")}
        0b010 => {String::from("[bp + si]")}
        0b011 => {String::from("[bx + di]")}
        0b100 => {String::from("[si]")}
        0b101 => {String::from("[di]")}
        0b110 => {String::from("[bp]")}
        0b111 => {String::from("[bi]")}
        _ => {panic!("Unimplemented code {0:b}!", code)}
    }
}

fn main() {
    let emu: Emu8086 = argh::from_env();
    let data = std::fs::read(emu.file).expect("Failed to open file");

    if emu.disassemble {
        let instruction_mask = 0b11111100;
        let mod_mask = 0b11000000;
        let reg_mask = 0b00111000;
        let rm_mask = 0b00000111;
        let d_mask = 0b00000010;
        let width_mask = 0b00000001;

        println!("bits 16");

        let mut i = 0;

        while i < data.len() {
            let mut instruction_code = (data[i] & instruction_mask) >> 2;
            let d = (data[i] & d_mask) >> 1;
            let mut width = data[i] & width_mask;
            let rm;
            let reg;

            let regmod : u8;
            let mut immediate_data : u16 = 0;

                //short immediate instructions
            if instruction_code & 0b111100 == 0b101100 {
                instruction_code = instruction_code >> 2;
                width = (data[i] & 0b00001000) >> 3;
                reg = data[i] & 0b00000111;
                i += 1;
                immediate_data = data[i] as u16;

                if width == 1 {
                    i += 1;
                    immediate_data |= (data[i] as u16) << 8;
                }

                //println!("op:{0:b} D:{1:b} W:{2:b} MOD:{3:b} REG:{4:b} (imdata: {5})", instruction_code, d, width, regmod, reg, immediate_data);
                println!("{0} {1}, {2}", instruction_bin_to_mnemonic(instruction_code),
                         reg_bin_to_mnemonic(reg, width == 1),
                         immediate_data);
            }
                //accumulator
            else if instruction_code == 0b101000 {
                let mut source_str: String;
                let mut dest_str: String;

                i += 1;
                let mut address : u16 = data[i] as u16;
                i += 1;
                address |= (data[i] as u16) << 8;

                dest_str = String::from("ax");
                source_str = String::from(format!("[{0}]", address));

                if d == 1 {
                    swap(& mut source_str, & mut dest_str);
                }

                println!("op:{0:b} D:{1:b} W:{2:b} ADDR:{3})", instruction_code, d, width, address);

                println!("{0} {1}, {2}", instruction_bin_to_mnemonic(instruction_code),
                         dest_str,
                         source_str);
            }
            else {
                i += 1;
                regmod = (data[i] & mod_mask) >> 6;
                rm = data[i] & rm_mask;
                reg = (data[i] & reg_mask) >> 3;

                let mut source_str: String;
                let mut dest_str: String;
                let mut instr_str: String;

                //weird logic for arithmetic ops with immediates
                if instruction_code == 0b100000 {
                    match reg {
                        0b000 => {instr_str = String::from("add")}
                        0b101 => {instr_str = String::from("sub")}
                        0b111 => {instr_str = String::from("cmp")}
                        _ => {panic!("Unimplemented arithmatic immediate case {0}!", reg)}
                    }
                }
                else {
                    instr_str = instruction_bin_to_mnemonic(instruction_code);
                }

                match regmod {
                    0b11 => {
                        source_str = reg_bin_to_mnemonic(reg, width == 1);
                        dest_str = reg_bin_to_mnemonic(rm, width == 1);
                    }
                    0b00 => {
                        source_str = reg_bin_to_mnemonic(reg, width == 1);

                        if rm != 0b110 {
                            dest_str = bin_to_address_calc(rm);
                        }
                        else {
                            i += 1;
                            let mut displacement : u16 = data[i] as u16;
                            i += 1;
                            displacement |= (data[i] as u16) << 8;
                            dest_str = format!("[{0}]", displacement)
                        }
                    }
                    0b01 => {
                        source_str = reg_bin_to_mnemonic(reg, width == 1);
                        dest_str = bin_to_address_calc(rm);
                        i += 1;
                        let displacement = data[i];

                        if displacement > 0 {
                            dest_str.pop();
                            dest_str.push_str(" + ");
                            dest_str.push_str(displacement.to_string().as_str());
                            dest_str.push(']');
                        }
                    }
                    0b10 => {
                        source_str = reg_bin_to_mnemonic(reg, width == 1);
                        dest_str = bin_to_address_calc(rm);
                        i += 1;
                        let mut displacement : u16 = data[i] as u16;
                        i += 1;
                        displacement |= (data[i] as u16) << 8;

                        if displacement > 0 {
                            dest_str.pop();
                            dest_str.push_str(" + ");
                            dest_str.push_str(displacement.to_string().as_str());
                            dest_str.push(']');
                        }
                    }
                    _ => {panic!("Unknown MOD {0}!", regmod)}
                }

                if instruction_code == 0b110001 {
                    i += 1;
                    immediate_data = data[i] as u16;

                    if width == 1 {
                        i += 1;
                        immediate_data |= (data[i] as u16) << 8;
                        source_str = format!("word {0}", immediate_data);
                    }
                    else {
                        source_str = format!("byte {0}", immediate_data);
                    }
                }
                else if d == 1 {
                    swap(& mut source_str, & mut dest_str);
                }

                println!("op:{0:b} D:{1:b} W:{2:b} MOD:{3:b} REG:{4:b} R/M: {5:b} (imdata: {6:b})", instruction_code, d, width, regmod, reg, rm, immediate_data);

                println!("{0} {1}, {2}", instr_str,
                         dest_str,
                         source_str);

            }

            i += 1;
        }
    }
}
