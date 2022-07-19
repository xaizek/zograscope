NAME
====

*zs-hi* is a primitive code highlighter for terminal.  The main purpose is
testing and debugging of code parsing, but it can also be used on its own.

The only hard-coded color scheme at the moment is for xterm-256color palette.

INVOCATION
==========

`zs-hi` `[options...]` _[{path|-}]_

Not providing _path_ argument is equivalent to specifying `-` and thus reading
from standard input.

EXAMPLES
========

Highlighting standard input
---------------------------

```
pahole -C key_chunk_t vifm | zs-hi
```

Highlighting a file
-------------------

```
zs-hi utils.c
```

Checking if code parses correctly
---------------------------------

Whether the parser is able to handle given sources can be checked like this:

```
find -name '*.[hc]' -exec zs-hi --dry {} \;
```

It produces output for files with which it has issues.

SEE ALSO
========

**zograscope**(7) for common options and list of all tools there.
