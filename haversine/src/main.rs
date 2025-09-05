use rand::prelude::*;
use std::fmt::Write;
use std::time::Instant;
use argh::FromArgs;
use rand_chacha::ChaCha8Rng;
use serde::Deserialize;

//#[global_allocator]
//static ALLOC: snmalloc_rs::SnMalloc = snmalloc_rs::SnMalloc;
#[derive(FromArgs)]
/// produces pairs of spherical coordinates and computes haversine on them
struct Haversine {
    #[argh(switch, short = 'i')]
    /// forces input file regeneration
    regen : bool,
    #[argh(option, default = "123")]
    /// seed for input generation
    seed : u64,
    #[argh(option, default = "10000")]
    /// number of pairs. Ignonred if input file is not generated
    numpairs : usize,
    #[argh(option, short = 'f', default = "String::from(\"input.json\")")]
    /// seed for input generation
    input_filename : String
}

fn geninput(outfile_name : &str, seed : u64, numpairs : usize) {
    let charsperpair = "{\"x0\":102.1633205722960440, \"y0\":-24.9977499718717624, \"x1\":-14.3322557404258362, \"y1\":62.6708294856625940},".len();
    let mut outstr = String::from("{\"pairs\":[");
    let mut rng = ChaCha8Rng::seed_from_u64(seed);

    outstr.reserve(numpairs * charsperpair * "]}".len());

    for i in 0..numpairs {
       write!(&mut outstr, "{{\"x0\":{0}, \"y0\":{1}, \"x1\":{2}, \"y1\":{3}}},",
           rng.random_range(-180.0..=180.0),
           rng.random_range(-90.0..=90.0),
           rng.random_range(-180.0..=180.0),
           rng.random_range(-90.0..=90.0)).expect(format!("Writing pair #{0} failed!", i).as_str());
    }

    outstr.pop();
    outstr.push_str("]}");

    std::fs::write(outfile_name, outstr).expect("Failed to write the file!");
}

fn reference_haversine(x0 : f64, y0 : f64, x1 : f64, y1 : f64, earth_radius: f64) -> f64 {
    /* NOTE(casey): This is not meant to be a "good" way to calculate the Haversine distance.
      Instead, it attempts to follow, as closely as possible, the formula used in the real-world
      question on which these homework exercises are loosely based.
   */
    let lat1 = y0;
    let lat2 = y1;
    let lon1 = x0;
    let lon2 = x1;

    let d_lat = (lat2 - lat1).to_radians();
    let d_lon = (lon2 - lon1).to_radians();
    let lat1 = lat1.to_radians();
    let lat2 = lat2.to_radians();

    let sin_lat = (d_lat /2.0).sin(); //wtf why did they make trig functions like this
    let sin_lon = (d_lon /2.0).sin();
    let a = sin_lat * sin_lat + lat1.cos() * lat2.cos() * sin_lon * sin_lon;
    let c = 2.0 * a.sqrt().asin();

    earth_radius * c
}

#[derive(Deserialize)]
struct Pair {
    x0 : f64,
    y0 : f64,
    x1 : f64,
    y1 : f64,
}

#[derive(Deserialize)]
struct Pairs {
    pairs : Vec<Pair>
}

fn run_haversine_on_file(file_name : &String) {
    println!("Parsing {0}", file_name);
    let now = Instant::now();
    let pairs : Pairs = simd_json::from_reader(std::fs::File::open(file_name).unwrap()).unwrap();
    let elapsed = now.elapsed();

    println!("Parsed {0} in {1:.2?}", file_name, elapsed);

    println!("Haversining {0}", file_name);
    let now = Instant::now();
    let mut sum = 0.0;

    for pair in pairs.pairs {
        sum += reference_haversine(pair.x0, pair.y0, pair.x1, pair.y1, 6372.8);
    }
    let elapsed = now.elapsed();

    println!("Haversined {0} in {1:.2?}", file_name, elapsed);

    println!("Found a sum of: {0}", sum);
}

fn main() {
    let have : Haversine = argh::from_env();

    if have.regen || !std::fs::exists(&have.input_filename).unwrap() {
        println!("Generating {0}...", have.input_filename);
        let now = Instant::now();

        geninput(&have.input_filename, have.seed, have.numpairs);

        let elapsed = now.elapsed();
        println!("Generated {0} in {1:.2?}", have.input_filename, elapsed);
    }

    run_haversine_on_file(&have.input_filename);
}
