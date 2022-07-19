NAME
====

`zs-tui` is a TUI version for processing files, whose scope is yet to be
defined.  So far it lists files or functions along with their size and parameter
count and allows viewing those items in source code as well as viewing dump of
their internal representation.

It's not clear if it's worth adding diffing functionality here.  It's probably
not, which means that this tool will be mostly for interactive browsing or
similar activities and other tools might be extracted out of it.

INVOCATION
==========

`zs-tui` `[options...]` _[paths...]_

Paths can specify both files and directories.  When no path is specified, "." is
assumed.

Tool-specific Options
---------------------

`--lang` \
here this common option also limits set of files to process

CONTROLS
========

Supported Vim-like shortcuts:

 * `G` -- to last line
 * `gg` -- to first line
 * `j` -- to the item below
 * `k` -- to the item above

Other shortcuts:

 * *files* view:
    - `c` -- enter code view
    - `d` -- enter dump view
    - `f` -- switch to functions view
    - `q` -- quit the application
 * *functions* view:
    - `c` -- enter code view
    - `d` -- enter dump view
    - `f` -- switch to files view
    - `q` -- quit the application
 * *code* view:
    - `c/q` -- leave code view
 * *dump* view:
    - `d/q` -- leave dump view

SEE ALSO
========

**zograscope**(7) for common options and list of all tools there.
