NAME
====

*zs-diff* is a terminal-based syntax-aware diff.  The syntax-aware part means
that comparison isn't sensible to formatting changes and understands general
structure of its input.

The primary purpose of the utility is to be used as external diff by `git`.

INVOCATION
==========

Manual Form
-----------

`zs-diff` `[options...]` _old-file_ _new-file_

To be used in a shell.

Git Form
--------

`zs-diff` `[options...]` _path_ _old-file_ _old-hex_ _old-mode_
                                _new-file_ _new-hex_ _new-mode_

When Git calls external diff for a changed file.

Git Rename Form
---------------

`zs-diff` `[options...]` _path_ _old-file_ _old-hex_ _old-mode_
                                _new-file_ _new-hex_ _new-mode_
                                _new-path_ _rename-msg_

When Git calls external diff for renamed and possibly changed file.

USAGE AND BEHAVIOUR
===================

Invoking manually
-----------------

Just do:

```bash
zs-diff old-file new-file
```

When Invoked by Git
-------------------

If parsing fails when Git-like invocation is used, the tool attempts to invoke
`git` with `--no-ext-diff` to do the diff.  Unfortunately, Git doesn't supply
enough information for a proper invocation and `git`-header you'll see will be
with blob hashes or temporary paths.

Use with Git without Configuration
----------------------------------

Use `$GIT_EXTERNAL_DIFF`:

```bash
GIT_EXTERNAL_DIFF='zs-diff --color' git show --ext-diff
```

Local Git integration
---------------------

Add `zs-diff` as a diff tool to `git` with these lines (`.git/config`):

```gitconfig
[diff "zs-diff"]
    command = zs-diff --color
```

Then configure which files it should be used for (`.git/info/attributes`):

```gitattributes
*.[ch]        diff=zs-diff
*.h.in        diff=zs-diff

*.[ch]pp      diff=zs-diff

Makefile      diff=zs-diff
Makefile.am   diff=zs-diff
Makefile.win  diff=zs-diff

*.lua         diff=zs-diff

*.bash        diff=zs-diff
*.sh          diff=zs-diff
```

This will make it work for `git diff` and `git show`, but `git log` and other
subcommands don't use custom diff tools by default and need `--ext-diff` option
to be specified (can be done in an alias).

Global Git integration
----------------------

Same as above, but specify attributes in `~/.config/git/attributes` and use one
of the following files for configuration:

 * `~/.config/git/config`
 * `~/.gitconfig`

Explicit use in Git
-------------------

Interactive (say `y` to view specific file):

```bash
git difftool -x zs-diff
```

SEE ALSO
========

**zograscope**(7) for common options and list of all tools there.
