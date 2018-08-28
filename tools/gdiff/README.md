**[zograscope][zograscope] :: zs-gdiff**

![Screenshot](data/example/screenshot.png)

## Description ##

`zs-gdiff` is a GUI syntax-aware diff that uses Qt5.  See [description of
zs-diff][zs-diff] for more details.

### Status ###

There are some issues due to Qt's support for displaying code being in a quite
bad state.  However, the tool is usable.  The issues could be resolved or
different GUI toolkit could be used in the future.

While `git` format of command-line is supported, using external GUI tools from
`git` isn't as convenient as one might wish.  That's why this tool will probably
get ability to query data from `git` directly.

[zograscope]: ../../README.md
[zs-diff]: ../diff/README.md
