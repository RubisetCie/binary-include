# Binary Include

**Binary Include** (*binclude*) is a simple command-line utility that turns binary files (or any type of file) into C source files. It can be used to embed external files into C or C++ programs.

## Building

*Binary Include* is a standard C program, which must be built using `cmake`:

```
cmake .
```

You may input various format options, to drive the generation of source files:

```
cmake \
  -DFMT_HEADER="<warning-header...>"  \
  -DFMT_NAME_COMMENT="File: %s"       \
  -DFMT_SIZE_SUFFIX="size"            \
  -DFMT_SIZE_TYPE="long"              \
  -DFMT_DATA_TYPE="unsigned char"     \
  -DFMT_DATA_INDENT "    "            \
  -DFMT_DATA_PER_LINE 12 .
```

## Usage

The `binclude` tool support the following options:

- `-o`, `--output`: Specify the output file (can be source or header).
- `-d`, `--header`: Specify a header file (won't be created otherwise).
- `-w`, `--no-warning`: Suppress the auto-generated warning comment in output.
- `-a`, `--no-allman`: Disable the Allman style of indentation and use the alternative K&R.
- `-f`, `--decimal`: Format byte data as decimal rather than the default hexadecimal.
- `-t`, `--text`: Write data as a text form rather than byte per byte.
- `-m`, `--macro`: Create the size definition as a macro instead of a const.
- `-c`, `--camel-case`: Use the camel case for names instead of the default snake case.
- `-s`, `--single-line`: Put all the data on a single line.
- `-v`, `--version`: Print program version.

### Examples

```
binclude -o foo.h bar.bin
binclude -o foo.h file1 file2
binclude -o foo.c -d foo.h bar.bin
```
