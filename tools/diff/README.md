**[zograscope][zograscope] :: zs-diff**

![Screenshot](../../data/examples/cxx/screenshot.png)

1. [Description](#description)
2. [Comparison](#comparison)
3. [Running without integration](#running-without-integration)
4. [Integrating into Git](#integrating-into-git)

## Description ##

`zs-diff` is a terminal-based syntax-aware diff.  The syntax-aware part means
that comparison isn't sensible to formatting changes and understands general
structure of its input.

The primary purpose of the utility is to be used as external diff within `git`.

### Status ###

Complicated changes of expressions or rewrites of functions might produce
results that are hard to understand.  Small or medium changes should be mostly
handled well.

## Documentation ##

See the [manual page][manual] for more details.

## Comparison ##

(This section is outdated and needs an update.)

This section presents various kinds of alternative diffs and demonstrates why
syntax-aware ones are useful.

The example was crafted to be small, non-trivial (all tools handle trivial
changes gracefully) and demonstrate all four kinds of changes (deletion,
insertion, update and move).  It partially avoids cases which due to current
heuristics don't look that nice in `zs-diff`, but is otherwise objective (those
cases are usually less common and aren't actually handled well by other tools
either).

#### `git-diff` ####

![git-diff](data/example/screenshots/git-diff.png)

Complete removal and addition isn't bad on its own, but it forces to match lines
manually and look for changes in them manually too
([diff-highlight][diff-highlight] script doesn't work in cases like this one).

#### `diff --side-by-side` ####

![sdiff](data/example/screenshots/sdiff.png)

Somewhat more readable version of `git-diff` even though it lacks highlighting.

#### [cdiff][cdiff] ####

![cdiff](data/example/screenshots/cdiff.png)

The alignment is good.  Highlighting within lines is somewhat messy.

#### [icdiff][icdiff] ####

![icdiff](data/example/screenshots/icdiff.png)

This is quite readable actually, but again character-level diffing is hard to
understand.

#### [ydiff][ydiff] ####

![ydiff](data/example/screenshots/ydiff.png)

The granularity of detected changes is coarse, but changes can be identified.

#### gumtree ####

![gumtree](data/example/screenshots/gumtree.png)

Disregarding unfortunate colors it's possible to see that structural changes
were in fact captured albeit not perfectly.

#### zs-diff ####

![zs-diff](data/example/screenshots/zs-diff.png)

Fine-grained matching would benefit from improvements as well as change
detection for more complicated cases in general, but changes that were applied
to this piece of code are quite easy to see and reason about.

[zograscope]: ../../README.md

[manual]: ../../docs/zs-diff.md
[diff-highlight]: https://github.com/git/git/tree/master/contrib/diff-highlight
[cdiff]: https://github.com/ymattw/cdiff
[icdiff]: https://www.jefftk.com/icdiff
[ydiff]: https://github.com/yinwang1/ydiff
