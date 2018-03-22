**[zograscope][zograscope] :: zs-find**

![Screenshot](data/example/screenshot.png)

## Description ##

`zs-find` is a tool for searching in code.  Applications include things like
collecting crude statistics about the code, looking for certain nested
configurations or sequences tokens.

The syntax and matching abilities are quite basic and are subject to significant
changes in the future.

## Examples ##

List all comments in C files:

```
zs-find --lang c : dir
```

Count number of preprocessor directives inside functions in all supported
languages:

```
zs-find --count src/ : func dir
```

Print functions of a specific file if they contain `return` statement:

```
zs-find src/utils/time.hpp : func : return
```

[zograscope]: ../../README.md
