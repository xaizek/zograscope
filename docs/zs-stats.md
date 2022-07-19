NAME
====

_zs-stats_ is a tool for counting:

 * lines of code
 * size of functions (complete definition)
 * number of functions
 * number of parameters of functions
 * possibly other things in the future

INVOCATION
==========

`zs-stats` `[options...]` _[paths...]_

Paths can specify both files and directories.  When no path is specified, "." is
assumed.

Tool-specific Options
---------------------

`--annotate` \
print source code annotated with line types

`--lang` \
here this common option also limits set of files to process

OPERATION
=========

Differences from some similar tools:

 * counts line containing only braces/brackets/parenthesis separately from code
 * blank lines inside multiline comments are counted as comments
 * blank lines inside string literals are counted as code
 * can be slower due to parsing source code instead of treating it as text
 * supports a few languages and new ones are harder to add
 * trailing blank lines are ignored (because there are no tree nodes for them,
   although it's possible to add them)
 * last line continuation in macros in C aren't recognized due to lack of
   tokenization of preprocessor directives; in C++ it might not be recognized
   due to SrcML bugs

SEE ALSO
========

**zograscope**(7) for common options and list of all tools there.
