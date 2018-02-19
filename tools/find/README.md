**[zograscope][zograscope] :: zs-find**

![Screenshot](data/example/screenshot.png)

## Description ##

`zs-find` is a tool for searching in code.  Applications include things like
collecting crude statistics about the code and looking for certain nested
configurations.

The syntax and matching abilities are very basic and are subject to significant
changes in the future.

## Examples ##

Listing all comments in C files:

```
zs-find --lang c . dir
```

Counting number of preprocessor directives inside functions in all supported
languages:

```
zs-find --count src/ func dir
```

[zograscope]: ../../README.md
