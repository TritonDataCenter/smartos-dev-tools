splitpatch
----------
This breaks a unified patch file (as exported from something like
"git format-patch") and breaks it into a seperate patchfile per file
being patched.

It was inspired by https://github.com/jaalto/splitpatch, which is a ruby
script I found on github.  Problem was, that script didn't work for me - it
produced malformed files that "git apply/am" rejected.  I don't write ruby and
so had little inclination to fix it.  Originally, I rewrote the script in PERL.

Later on, as a practice exercise, I rewrote it in Rust.  And that is what is
contained here.
