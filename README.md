# tipyconv

TI-84 Plus CE Python tools for converting Python files to and from the proprietary 8Xv format, and some tools.

## Installation

 * Run `make`
 * Copy `tipyconv` to whichever prefix you like.

## Running

To convert one format to another:

```
tipyconv SCRIPT.8xv
```

You can also specify an output path with `-o`.

The output format will be automatically detected. For more information, consult the `--help` screen, or run the program with no arguments.
