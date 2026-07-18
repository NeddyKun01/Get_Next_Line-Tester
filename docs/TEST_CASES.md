# Test Cases

This document lists the focused cases accepted by `--only`, `--skip`,
`--explain`, and `--export-fixture`.

| Case | Suite | Input shape | Expected behavior | Common failures |
| --- | --- | --- | --- | --- |
| `invalid-fd` | mandatory, bonus | `get_next_line(-1)` | Returns `NULL`. | Reads invalid fd, keeps stale state. |
| `closed-fd` | mandatory | Open, close, then read fd. | Returns `NULL`. | Uses stale fd state. |
| `empty` | mandatory | Empty file. | Returns `NULL`, then `NULL`. | Returns empty string. |
| `one-line` | mandatory | One line with and without final newline. | Preserves final line semantics. | Drops EOF line or strips newline. |
| `many` | mandatory | Several regular lines. | Returns lines in order. | Duplicates or skips bytes. |
| `blank` | mandatory | Mixed blank and nonblank lines. | Returns `"\n"` for blank lines. | Treats blank lines as EOF. |
| `newline-only` | mandatory | A file containing only `"\n"`. | Returns one blank line. | Drops the newline. |
| `many-newlines` | mandatory | A file containing only newlines. | Returns every blank line. | Collapses blank lines. |
| `long` | mandatory | A 200-byte line plus tail. | Joins chunks correctly. | Slow joins, truncation, leaks. |
| `buffer-edge` | mandatory | `BUFFER_SIZE - 1`, `BUFFER_SIZE`, `BUFFER_SIZE + 1`. | Keeps every byte at boundaries. | Off-by-one stash handling. |
| `double-buffer` | mandatory | `BUFFER_SIZE * 2`, `BUFFER_SIZE * 2 + 1`. | Handles exact multi-buffer boundaries. | Exact-read EOF bugs. |
| `read-zero` | mandatory | Repeated invalid fd calls. | Returns `NULL` consistently. | Keeps stale data after read errors. |
| `malloc-fail` | mandatory | First allocation failure with `--malloc-fail`. | Returns `NULL` cleanly. | Crashes or leaks partial state. |
| `pipe` | mandatory | Pipe-backed fd. | Reads like any valid fd. | Assumes regular files only. |
| `stdin` | mandatory | Fixture redirected into fd `0`. | Reads stdin correctly. | Rejects fd `0`. |
| `stress` | mandatory | 10k and 100k line fixtures. | Handles large lines. | Timeouts, quadratic joins. |
| `bonus-basic` | bonus | Two interleaved fds. | Keeps independent state. | One global stash. |
| `bonus-many-fds` | bonus | Eight round-robin fds. | Tracks EOF and lines per fd. | Wrong fd index cleanup. |
| `bonus-wide-fds` | bonus | Configurable wide fd set. | Handles many fds. | Too-small fd storage. |

Focused examples:

```sh
./gnl_tester --root ../Get_Next_Line --only double-buffer --quick
./gnl_tester --root ../Get_Next_Line --skip stress --strict
./gnl_tester --root ../Get_Next_Line --only malloc-fail --quick
./gnl_tester --export-fixture buffer-edge --buffer 42 --output exported-fixtures
```

