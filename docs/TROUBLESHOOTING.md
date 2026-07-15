# Troubleshooting

## Missing Target Files

If the tester prints a missing file error, check that `--root` points to the
folder containing your source files.

Mandatory mode expects:

```text
get_next_line.c
get_next_line_utils.c
get_next_line.h
```

Bonus mode expects:

```text
get_next_line_bonus.c
get_next_line_utils_bonus.c
get_next_line_bonus.h
```

## Compile Failures

The tester compiles with:

```text
cc -Wall -Wextra -Werror -D BUFFER_SIZE=N
```

Common causes:

- missing includes in `get_next_line.h`;
- function prototypes that do not match the implementation;
- warnings promoted to errors by `-Werror`;
- helper functions declared with the wrong return type;
- code that only compiles for one specific `BUFFER_SIZE`.

Re-run the failing command with one buffer size to focus the problem:

```sh
./gnl_tester --root ../Get_Next_Line --buffer 1 --no-color
```

## Runtime Failures

Runtime failures usually mean a returned line differs from the expected string.

Common causes:

- newline is missing from returned lines;
- EOF line without final newline is discarded;
- stash is not cleared after EOF;
- bytes are duplicated or skipped across buffer boundaries;
- empty lines are treated as EOF.

## Bonus Failures

If mandatory passes but bonus fails, check that the bonus implementation keeps
separate state per file descriptor.

Common causes:

- using a single static pointer instead of fd-indexed storage;
- freeing or overwriting the wrong fd stash;
- assuming reads always happen from one fd until EOF.

## Leak Check Notes

`--leaks` requires Valgrind. If Valgrind is not installed, the tester still runs
functional checks and prints a skip note.

If Valgrind reports reachable or lost memory, inspect the path that returns
`NULL`, the path after EOF, and error paths after failed `read` or `malloc`.

## Paths With Spaces Or Accents

The tester quotes paths passed to compile and run commands. If a path still
fails, try an absolute path first:

```sh
./gnl_tester --root /absolute/path/to/Get_Next_Line --strict
```
