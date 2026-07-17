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

## Timeout Failures

If a run prints `timeout(3000ms)` or another configured value, the compiled test
case did not finish before the limit.

Common causes:

- infinite loops while reading until newline;
- not advancing or clearing the stash after EOF;
- retrying `read` forever after it returns `0` or `-1`;
- extremely slow string joining on long lines.

To confirm whether the issue is just a slow machine, raise the timeout:

```sh
./gnl_tester --root ../Get_Next_Line --strict --timeout 5000
```

Stress mode intentionally adds large 10k and 100k line fixtures. If stress mode
times out with tiny buffers such as `BUFFER_SIZE=1`, retry with a larger timeout
or a focused buffer first:

```sh
./gnl_tester --root ../Get_Next_Line --stress --buffer 42 --timeout 10000
```

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

The tester classifies common Valgrind failures before printing the full log:

| Category | Typical cause |
| --- | --- |
| `definitely lost` | Allocated memory is no longer reachable. |
| `indirectly lost` | Memory reachable only from leaked memory was also lost. |
| `possibly lost` | Pointer tracking became ambiguous. |
| `still reachable` | Memory remains reachable at exit. |
| `invalid read` | Reading freed or out-of-bounds memory. |
| `invalid write` | Writing freed or out-of-bounds memory. |
| `invalid free` | Freeing an invalid, duplicate, or mismatched pointer. |
| `uninitialised value` | Branching or output depends on uninitialised data. |

## Paths With Spaces Or Accents

The tester quotes paths passed to compile and run commands. If a path still
fails, try an absolute path first:

```sh
./gnl_tester --root /absolute/path/to/Get_Next_Line --strict
```
