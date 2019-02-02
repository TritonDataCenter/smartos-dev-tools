//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (c) 2019, Joyent, Inc.
//
extern crate getopts;

use getopts::Options;
use std::env;
use std::process;

extern crate splitpatch;

fn usage(progname: &str, opts: Options) {
    let msg = format!("Usage: {} -p <PATCHFILE> -o <OUTPUT_DIR>", progname);
    print!("{}", opts.usage(&msg));
} 

fn main() {
    let args: Vec<String> = env::args().collect();
    let progname = args[0].clone();

    let mut opts = Options::new();
    opts.optopt("p", "patch", "Patch file to split", "PATCHFILE");
    opts.optopt("o", "output-dir", "Output directory", "OUTPUT_DIR");
    opts.optflag("h", "help", "print usage message");

    let matches = match opts.parse(&args[1..]) {
        Ok(m) => {
            m
        }
        Err(e) => {
            panic!(e.to_string())
        }
    };

    if matches.opt_present("h") {
        usage(&progname, opts);
        return;
    }
    
    let patch_file = match matches.opt_str("p") {
        Some(p) => { p }
        None => {
            usage(&progname, opts);
            process::exit(2);
        }
    };

    let out_dir = match matches.opt_str("o") {
        Some(o) => { o }
        None => {
            usage(&progname, opts);
            process::exit(2);
        }
    };

    let config = splitpatch::Config::new(patch_file, out_dir);

    match splitpatch::run(config) {
        Ok(_r) => {
            println!("splitpatch succeeded!");
            process::exit(0);
        }
        Err(e) => {
            eprintln!("An error occurred: {}", e.to_string());
            process::exit(1);
        }
    }
}
