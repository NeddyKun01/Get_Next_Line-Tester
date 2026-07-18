# Coverage

This document describes the behavior covered by the current tester.

## Mandatory Mode

Mandatory mode compiles:

```text
get_next_line.c
get_next_line_utils.c
get_next_line.h
```

Covered cases:

| Case | What it checks |
| --- | --- |
| invalid fd | `get_next_line(-1)` returns `NULL`. |
| empty file | EOF is handled without allocating a fake line. |
| one line without final newline | Last line is returned without requiring `\n`. |
| one line with final newline | Newline is included in the returned line. |
| multiple lines | Consecutive calls return lines in order. |
| blank lines | Empty lines represented by `"\n"` are preserved. |
| long line | Lines larger than common buffer sizes are joined correctly. |
| exact-ish boundary fixture | Buffer edge behavior does not drop or duplicate bytes. |
| calls after EOF | Repeated calls after EOF keep returning `NULL`. |
| pipe fd | Lines can be read from a pipe file descriptor. |
| timeout | Stuck test runs are killed after the configured timeout. |

## Stress Mode

Stress mode is enabled with `--stress`.

| Case | What it checks |
| --- | --- |
| 10k line with newline and tail | Large line extraction, stash handoff, and following short line. |
| 100k line without final newline | Large EOF line handling without requiring `\n`. |
| 100k line with newline and tail | Large line cleanup before returning the next short line. |

## Bonus Mode

Bonus mode compiles:

```text
get_next_line_bonus.c
get_next_line_utils_bonus.c
get_next_line_bonus.h
```

Covered cases:

| Case | What it checks |
| --- | --- |
| invalid fd | `get_next_line(-1)` returns `NULL`. |
| two open fds | Independent stash/state per file descriptor. |
| interleaved reads | Alternating calls between fds return the correct next line. |
| blank line on one fd | Empty lines do not disturb another fd. |
| many open fds | Round-robin reads across eight fds keep independent state. |
| EOF on each fd | EOF is tracked independently. |

## Buffer Matrix

The tester can run every case across multiple `BUFFER_SIZE` values:

| Profile | Values |
| --- | --- |
| default | `1,2,3,5,8,16,32,42,128,1024` |
| quick | `1,42` |
| strict | `1,2,3,4,5,7,8,16,32,42,64,128,1024` |
| custom | selected with `--buffer LIST` |

## Review Mode

Review mode runs the strict mandatory matrix and, when bonus files exist, the
strict bonus matrix. It reports a compact suite score and a final verdict.

When `--leaks` is enabled, review mode also reports a compact Valgrind line:

```text
Valgrind: OK
Valgrind: NOK invalid read, definitely lost
Valgrind: SKIP not installed
```

## Output Controls

| Option | What it changes |
| --- | --- |
| `--summary-only` | Hides passing buffer-suite lines in normal mode and keeps review output compact. |
| `--fail-fast` | Stops the current suite after the first failing buffer suite. |

## Not Yet Covered

Planned future coverage:

- 1MB line fixtures;
- stdin redirection checks;
