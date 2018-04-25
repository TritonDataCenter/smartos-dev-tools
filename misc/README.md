splitpatch.pl
-------------
This breaks a unified patch file (as exported from something like
"git format-patch") and breaks it into a seperate patchfile per file
being patched.

It was inspired by https://github.com/jaalto/splitpatch, which is a ruby
script I found on github.  Problem was, that script didn't work for me - it
produced malformed files that "git apply" rejected.  I don't write ruby and
so had little inclination to fix it, so I decided it'd be quicker to just
rewrite it in a scripting language I'm more familiar with - PERL :p
