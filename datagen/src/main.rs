extern crate clap;
extern crate serde;
extern crate serde_json;
#[macro_use]
extern crate serde_derive;
extern crate rand;

use clap::{App, Arg};
use rand::distributions::Alphanumeric;
use rand::prelude::*;
use std::fs::{create_dir, File};
use std::io::{self, Write};

#[derive(Deserialize)]
struct FileContent {
    pub title: String,
    pub content: String,
}

#[derive(Deserialize)]
struct Options {
    pub years: Vec<u16>,
    pub departments: Vec<String>,
    pub authors: Vec<String>,
    pub texts: Vec<FileContent>,
}

fn generate_file_content(
    content: &FileContent,
    year: u16,
    rng: &mut ThreadRng,
    target: &str,
) -> io::Result<()> {
    let mut template = String::new();

    // Meta data
    template += &format!("Name: {}\n", content.title);
    template += &format!("Autor: {}\n", "Ayn Rand");
    template += &format!(
        "Erstellungsdatum: {d}.{m}.{y}\n",
        d = rng.gen_range(1, 27),
        m = rng.gen_range(1, 13),
        y = year
    );
    // TODO: document ID
    let doc_id: String = rng.sample_iter(&Alphanumeric).take(9).collect();
    template += &format!("Aktenzeichen: {}\n", doc_id);

    let tech = if rng.gen::<f32>() > 0.6 {
        "manuell"
    } else {
        "digital"
    };
    template += &format!("Digitalisierungstechnik: {}\n", tech);
    template += &format!("Digitalisierungsdatum: {}\n", 2012);

    template += "--------------------------\n";

    // Content
    template += &content.content;

    let mut file = File::create(target.to_string() + &content.title.to_lowercase() + ".txt")?;
    file.write_all(&template.into_bytes())?;

    Ok(())
}

fn main() -> io::Result<()> {
    let matches = App::new("Crime Campus Data Generator")
        .version("1.0")
        .author("Felix Wittwer <dev@felixwittwer.de>")
        .about("Generates a fake directory structure with fictive company data")
        .arg(
            Arg::with_name("config")
                .help("JSON file describing the structure and the data to use.")
                .required(true),
        )
        .arg(
            Arg::with_name("OUTPUT")
                .help("The folder that shall serve as the root folder for all output.")
                .required(true),
        )
        .get_matches();

    let config_path = matches.value_of("config").unwrap();
    let output_path = matches.value_of("OUTPUT").unwrap();

    let config: Options = serde_json::from_reader(File::open(config_path)?).unwrap();
    let mut rng = thread_rng();

    for year in config.years {
        let out_dir = String::from(output_path) + &format!("/{}/", year);
        create_dir(&out_dir)?;

        for dept in &config.departments {
            let target = out_dir.clone() + &format!("{}/", dept);
            create_dir(&target)?;

            for _ in 0..rng.gen_range(1, 9) {
                let index = rng.gen_range(0, config.texts.len());
                generate_file_content(&config.texts[index], year, &mut rng, &target)?;
            }
        }
    }

    Ok(())
}
