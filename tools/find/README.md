**[zograscope][zograscope] :: zs-find**

![Screenshot](data/example/screenshot.png)

## Description ##

`zs-find` is a tool for searching in code.  Applications include things like
collecting crude statistics about the code, looking for certain nested
configurations or sequences tokens.

The syntax and matching abilities are relatively basic and are subject to
significant changes in the future.

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

List all statements containing uses of `exec()` family of functions:

```
zs-find : stmt : '//^(execl[pe]?|execvp?e?)$/'
```

List all invocations of `snprintf` which have single token as the first
argument:

```
zs-find ../src : call : snprintf '(' // ,
```

[zograscope]: ../../README.md
