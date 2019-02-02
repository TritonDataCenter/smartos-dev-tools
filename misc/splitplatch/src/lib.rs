//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (c) 2019, Joyent, Inc.
//
extern crate regex;

use std::error::Error;
use std::fs::File;
use std::io::BufRead;
use std::io::BufReader;
use std::io::Write;

use regex::Regex;

#[derive(Debug)]
pub struct Config {
    pub patch_file: String,
    pub out_dir: String,
}

impl Config {
    pub fn new(patch_file: String, out_dir: String) -> Config {
        Config { patch_file, out_dir }
    }
}

fn dump_patch(patch: &Vec<String>, target: &String, outdir: &String)
    -> std::io::Result<()> {

    let filenm = format!("{}/{}", outdir, target.replace("/", "_"));
    let mut outfile = File::create(&filenm)?;
    
    println!("dumping patch for {} to {}", target, filenm);
    for line in patch {
        outfile.write_all(line.as_bytes())?;
    }
    Ok(())
}


//
// Nothing fancy - just find the beginning of the first diff, and start
// pushing the contents into a vector.  From then on, when we find the start
// of the next diff, we dump the contents of the vector to a patch file,
// clear the vector and rinse, repeat.
//
pub fn run(config: Config) -> Result<(), Box<dyn Error>> {
    let file = File::open(config.patch_file)?;
    let reader = BufReader::new(file);
    
    // regex matching the start of a file diff
    let patch_start_re = Regex::new(r"^diff ")?;
    // regex extracting the patch target filename
    let patch_filenm_re = Regex::new(r"(\S+)$")?;

    let mut first_patch_started = false;
    let mut patch = Vec::new();
    let mut patch_filenm = String::new();

    for r in reader.lines() {
        let line = match r {
            Ok(l) => { l }
            Err(e) => {
                return Err(Box::new(e));
            }
        };
 
        if patch_start_re.is_match(&line) {
            if first_patch_started {
                assert!(patch.len() > 1, "malformed patch!");
                dump_patch(&patch, &patch_filenm, &config.out_dir)?;
                patch.drain(..);
            } else {
                first_patch_started = true;
            }

            assert_eq!(patch.len(), 0, "expected patch vector to be empty");
            
            let caps = patch_filenm_re.captures(&line).unwrap();
            match caps.get(0) {
                Some (c) => {
                    patch_filenm = c.as_str()[2..].to_string();
                }
                None => {
                    panic!("Failed to determine patch target");
                }
            }
            patch.push(line.clone());
        } else if first_patch_started {
            patch.push(line.clone());
        }
    }

    // Pop the two-line end marker off and write out the last patch
    assert!(patch.len() > 2, "malformed patch!");
    patch.pop();
    patch.pop();
    dump_patch(&patch, &patch_filenm, &config.out_dir)?;

    Ok(())
}
