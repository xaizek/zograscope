NAME
====

*zs-gdiff* is a GUI syntax-aware diff that uses Qt5.  See **zs-diff**(1) for
more details and comparison against other tools.

The tool can either accept two files on command-line, be integrated with `git`
by its means (yet using external GUI tools from `git` isn't very convenient in
general) or pick up list of changed files in the repository (staged or unstaged
in index or from a commit) by itself.  The latter way of using `zs-gdiff` with
`git` doesn't require any configuration and allows going through files without
restarting it, unlike when `git` invokes external tools.

It works, but it's far from being polished.

INVOCATION
==========

Manual Form
-----------

`zs-gdiff` `[options...]` _old-file_ _new-file_

To be used in a shell.

Git Status Form
---------------

`zs-gdiff` [`--cached`]

To be used in a shell to view staged/unstaged changes.

Git Commit Form
---------------

`zs-gdiff` _git-reference_

To be used in a shell to view commit changes.

Git Form
--------

`zs-gdiff` `[options...]` _path_ _old-file_ _old-hex_ _old-mode_
                                 _new-file_ _new-hex_ _new-mode_

When Git calls external diff for a changed file.

Git Rename Form
---------------

`zs-gdiff` `[options...]` _path_ _old-file_ _old-hex_ _old-mode_
                                 _new-file_ _new-hex_ _new-mode_
                                 _new-path_ _rename-msg_

When Git calls external diff for renamed and possibly changed file.

Tool-specific Options
---------------------

`--cached` \
use staged changes instead of unstaged

SEE ALSO
========

**zograscope**(7) for common options and list of all tools there.
