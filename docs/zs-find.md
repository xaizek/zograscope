NAME
====

*zs-find* is a tool for searching in code.  Applications include things like
collecting crude statistics about the code, looking for certain nested
configurations or sequences of tokens.

The syntax and matching abilities are relatively basic and are subject to
significant changes in the future.

INVOCATION
==========

`zs-find` `[options...]` _[paths...]_ : _matchers..._

`zs-find` `[options...]` _[paths...]_ : _[matchers...]_ : _[expressions...]_

`zs-find` `[options...]` _[paths...]_ :: _expressions..._

Paths can specify both files and directories.  When no path is specified, "." is
assumed.

Either _matchers..._, _expressions..._ or both must be specified.

Tool-specific Options
---------------------

`-c`, `--count` \
only count matches and report statistics

`--lang` \
here this common option also limits set of files to process

Matchers
--------

| Matcher | Description
|---------|-------------
| `decl`  | Any sort of declaration
| `stmt`  | Statements
| `func`  | Functions (their definitions only)
| `call`  | Function invocations
| `param` | Parameters of a function
| `comm`  | Comments of any kind
| `dir`   | Preprocessor-alike directives
| `block` | Containers of statements

Expressions
-----------

Each expressions matches a single token.

| Expr    | What it matches
|---------|-----------------
| `x`     | Exactly `x`
| `/^x/`  | Any token that starts with `x`
| `/x$/`  | Any token that ends with `x`
| `/^x$/` | Exactly `x`
| `/x/`   | Any token that contains `x` as a substring
| `//x/`  | Regular expression `x`
| `//`    | Any token

EXAMPLES
========

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

SEE ALSO
========

**zograscope**(7) for common options and list of all tools there.
