# Troubleshooting

## First Diagnostic Step

Run doctor mode before debugging a setup problem:

```sh
./gnl_tester --root ../Get_Next_Line --doctor
```

Doctor mode checks required tools, local tester harness files, mandatory files,
bonus files, header guards, `get_next_line` prototypes, optional Valgrind
availability, and Makefile shape when present. If it prints `NOK`, fix those
items before investigating functional failures.

Use `--diagnose` when you only want target-project checks:

```sh
./gnl_tester --root ../Get_Next_Line --diagnose
```

Use `--health` when you want a compact summary for a quick shell check:

```sh
./gnl_tester --root ../Get_Next_Line --health
```

Use `--doctor --json` when CI or another script needs structured diagnostics:

```sh
./gnl_tester --root ../Get_Next_Line --doctor --json
```

Use `--doctor --web` when you need a diagnostic file to attach to an issue:

```sh
./gnl_tester --root ../Get_Next_Line --doctor --web --output gnl-doctor.html
```

## Header Integrity Failures

If doctor mode reports a header failure, check that `get_next_line.h` contains:

```c
char	*get_next_line(int fd);
```

The header should also have an include guard or `#pragma once`. Bonus mode uses
the same expectation for `get_next_line_bonus.h`.

If header integrity passes but the header probe fails, make sure the header can
be included from a clean C file and exposes `get_next_line` without depending on
private implementation state.

## Makefile Warnings Or Failures

The tester compiles source files directly, so a missing target Makefile is a
warning rather than a hard failure. If a Makefile exists, doctor mode checks that
it mentions the expected source files and has a normal build target shape.

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

If you already know the failing case, use `--only`:

```sh
./gnl_tester --root ../Get_Next_Line --only buffer-edge --buffer 1 --no-color
```

## Runtime Failures

Runtime failures usually mean a returned line differs from the expected string.

Common causes:

- newline is missing from returned lines;
- EOF line without final newline is discarded;
- stash is not cleared after EOF;
- bytes are duplicated or skipped across buffer boundaries;
- empty lines are treated as EOF.

Use `--explain CASE` before a focused rerun when the case label is unclear:

```sh
./gnl_tester --cases
./gnl_tester --explain stdin
./gnl_tester --root ../Get_Next_Line --only stdin --quick --no-color
```

Use `--skip CASE` when one noisy case is hiding other failures:

```sh
./gnl_tester --root ../Get_Next_Line --skip stress --strict --no-color
```

If `--only` or `--skip` reports an unknown case, run `--cases` and copy the
case name exactly.

Use `--export-fixture CASE` when you want to inspect the input file manually:

```sh
./gnl_tester --export-fixture double-buffer --buffer 42 --output exported-fixtures
```

Use `--rerun-failed FILE` after a JSON report to focus on failed buffers:

```sh
./gnl_tester --root ../Get_Next_Line --rerun-failed gnl-test-report.json
```

## Pipe Failures

The mandatory suite includes a pipe-backed fd case. If only the pipe case fails,
look for code that assumes a regular file or depends on file-specific behavior
instead of treating the fd as a normal `read` source.

## Stdin Failures

The mandatory suite redirects a fixture file into `STDIN_FILENO` and reads it
with `get_next_line(0)`. If only stdin fails, check for code that rejects fd `0`
or treats it differently from other readable file descriptors.

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
- allocating too few fd slots for several open files;
- assuming reads always happen from one fd until EOF.

## Leak Check Notes

`--leaks` requires Valgrind. If Valgrind is not installed, the tester still runs
functional checks and prints a skip note.

If Valgrind reports reachable or lost memory, inspect the path that returns
`NULL`, the path after EOF, and error paths after failed `read` or `malloc`.

Use `--only malloc-fail --quick` for a focused allocation-failure smoke check.

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

## Empty Failure Output

If a compiled test binary exits with a failure status and no output, the tester
prints a fallback message in the terminal, JSON report, and Web dashboard. This
usually means the target crashed before the harness could print its `NOK` line,
or exited early from inside project code.

Web reports also include a short likely-fix hint for failing rows. Treat it as a
debugging starting point, then confirm with a focused terminal rerun.

## Inspecting Build Logs

Per-run build directories are cleaned automatically after each run. If you need
to inspect `compile.log`, `test.log`, generated fixtures, or the compiled
harness binary, rerun with:

```sh
./gnl_tester --root ../Get_Next_Line --buffer 42 --keep-build
```

Then inspect the newest directory under `tester/build/`.

## Compare Mode

If `--compare PATH` reports `TARGET WORSE`, rerun both roots with one focused
buffer size and no color:

```sh
./gnl_tester --root ../Get_Next_Line --compare ../Get_Next_Line-before --buffer 42 --no-color
```

Compare mode currently prints terminal output only. Do not combine it with
`--json`, `--web`, or `--html`.

## Config File

If `.gnl-tester.json` seems to apply unexpected options, rerun the command with
explicit CLI flags. CLI flags override config-file defaults.

Example minimal config:

```json
{
  "root": "../Get_Next_Line",
  "preset": "review"
}
```

## Output Files

`--output FILE` only works with `--json`, `--web`, or `--html`:

```sh
./gnl_tester --root ../Get_Next_Line --preset ci --output gnl-test-report.json
./gnl_tester --root ../Get_Next_Line --preset web --output gnl-test-report.html
```

If the output file cannot be created, check that the parent directory exists and
that your user can write there.
