# Recommended C style and coding rules

This document describes C code style used by this repository.

## The single most important rule

Let's start with the quote from [GNOME developer](https://developer.gnome.org/programming-guidelines/stable/c-coding-style.html.en) site.

> The single most important rule when writing code is this: *check the surrounding code and try to imitate it*.
>
> As a maintainer it is dismaying to receive a patch that is obviously in a different coding style to the surrounding code. This is disrespectful, like someone tromping into a spotlessly-clean house with muddy shoes.
>
> So, whatever this document recommends, if there is already written code and you are patching it, keep its current style consistent even if it is not your favorite style.

## General rules

Here are listed most obvious and important general rules. Please check them carefully before you continue with other chapters.

- Use `C89` standard
- Do not use spaces, use tabs instead
- Use `1` tab per indent level
- Use `1` space between keyword and opening bracket
```c
/* OK */
if (condition)
while (condition)
for (init; condition; step)
do {} while (condition)

/* Wrong */
if(condition)
while(condition)
for(init;condition;step)
do {} while(condition)
```

- Do not use a space between function name and opening bracket
```c
int a = sum(4, 3);              /* OK */
int a = sum (4, 3);             /* Wrong */
```

- Never use `__` or `_` as a prefix for variables/functions/macros/types. This is reserved for the C language itself
    - Prefer `prv_` name as a prefix for strictly module-private functions
- Use only lowercase characters for variables/functions/macros/types with an optional underscore `_` character
- Opening curly bracket is always at the same line as keyword (`for`, `while`, `do`, `switch`, `if`, ...)
```c
int i;
for (i = 0; i < 5; ++i) {           /* OK */
}
for (i = 0; i < 5; ++i){            /* Wrong */
}
for (i = 0; i < 5; ++i)             /* Wrong */
{
}
```

- Use single space before and after comparison and assignment operators
```c
int a;
a = 3 + 4;              /* OK */
for (a = 0; a < 5; ++a) /* OK */
a=3+4;                  /* Wrong */
a = 3+4;                /* Wrong */
for (a=0;a<5;++a)       /* Wrong */
```

- Use single space after every comma  (Opinions?)
```c
func_name(5, 4);        /* OK */
func_name(4,3);         /* Wrong */
```

- Do not initialize `static` and `global` variables to `0` (or `NULL`), let compiler do it for you
```c
static int a;       /* OK */
static int b = 4;   /* OK */
static int a = 0;   /* Wrong */

void
my_func(void) {
    static int* ptr;/* OK */
    static char abc = 0;/* Wrong */
}
```

- Declare all local variables of the same type in the same line
```c
void
my_func(void) {
    char a;             /* OK */
    char a, b;          /* OK */
    char b;             /* Wrong, variable with char type already exists */
}
```
