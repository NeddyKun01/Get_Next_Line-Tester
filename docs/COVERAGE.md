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
| closed fd | A previously opened and closed fd returns `NULL`. |
| empty file | EOF is handled without allocating a fake line. |
| one line without final newline | Last line is returned without requiring `\n`. |
| one line with final newline | Newline is included in the returned line. |
| multiple lines | Consecutive calls return lines in order. |
| blank lines | Empty lines represented by `"\n"` are preserved. |
| newline-only file | A file containing only `"\n"` returns a blank line before EOF. |
| many-newline file | A file made only of newline characters returns each blank line. |
| long line | Lines larger than common buffer sizes are joined correctly. |
| exact-ish boundary fixture | Buffer edge behavior does not drop or duplicate bytes. |
| generated buffer edges | Lines at `BUFFER_SIZE - 1`, `BUFFER_SIZE`, and `BUFFER_SIZE + 1`. |
| double buffer edges | Lines at `BUFFER_SIZE * 2` and `BUFFER_SIZE * 2 + 1`. |
| read-error repeat | Repeated invalid fd calls keep returning `NULL` without stale state. |
| malloc failure | Opt-in first-allocation failure returns `NULL` cleanly with `--malloc-fail`. |
| calls after EOF | Repeated calls after EOF keep returning `NULL`. |
| pipe fd | Lines can be read from a pipe file descriptor. |
| stdin fd | Lines can be read from `STDIN_FILENO` after stdin redirection. |
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
| wide open fds | Round-robin reads across 32 fds keep independent state. |
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
| `--json` | Emits machine-readable suite, result, timeout, and leak metadata. |
| `--web` | Emits a standalone HTML dashboard report. |
| `--html` | Alias for `--web`. |
| `--output FILE` | Writes JSON or Web output directly to a file. |
| `--preset NAME` | Applies a named workflow such as `quick`, `review`, `ci`, or `web`. |
| `--doctor` | Checks local tools, tester files, target layout, headers, prototypes, and Makefile shape. |
| `--doctor --json` | Emits structured diagnostics for CI and scripts. |
| `--doctor --web` | Emits a standalone HTML diagnostics report. |
| `--diagnose` | Runs target-project diagnostics without the local tool recommendation layer. |
| `--health` | Prints a compact health summary for automation or quick checks. |
| `--norm` | Runs Norminette diagnostics when available. |
| `--cases` | Lists valid focused case names. |
| `--explain CASE` | Explains what a named case checks and common failure causes. |
| `--only CASE` | Runs only one named harness case. |
| `--skip CASE` | Runs all selected harness cases except one named case. |
| `--export-fixture CASE` | Writes representative fixture files for manual debugging. |
| `--rerun-failed FILE` | Reruns failed buffers from a previous JSON report. |
| `--compact` | Prints a single-line terminal verdict. |
| `--profile` | Prints slowest buffer suites after a terminal run. |
| `--fd-limit N` | Controls the wide bonus fd count. |
| `--malloc-fail` | Enables malloc failure wrapper checks. |
| `--list` | Lists suites, presets, and report types. |
| `--coverage` | Prints tested behavior coverage. |
| `--coverage-md` | Prints tested behavior coverage as Markdown. |
| `--compare PATH` | Compares another target root with the same selected options. |
| `--keep-build` | Keeps per-run build directories for debugging. |

## JSON Output

JSON mode reports:

- target path and selected mode;
- timeout, stress, leak, and fail-fast settings;
- selected case filters;
- target diagnostics;
- selected `BUFFER_SIZE` values;
- pass/fail summary;
- per-buffer compile, runtime, timeout, leak status, failed case labels, and
  likely-fix hints;
- review-mode mandatory and bonus suite summaries;
- Valgrind issue categories when leak checks are enabled.
- tester version metadata.

## Web Dashboard

Web mode reports the same run result data in a standalone HTML file. It covers:

- target path, mode, timeout, stress, leak, and fail-fast settings;
- target diagnostics;
- suite-level mandatory and bonus scores;
- per-buffer compile, runtime, timeout, and leak status;
- per-buffer duration metadata;
- original command metadata;
- expandable failure output for failing rows;
- likely-fix hints for failing rows;
- all, passed, and failed row filters;
- copyable rerun commands for failing buffer suites.

## Operational Checks

The tester also covers its own workflows:

- unique per-process build directories for parallel report generation;
- automatic cleanup of per-run build directories by default;
- direct JSON and Web file output with `--output`;
- named presets for repeatable local, review, CI, Web, stress, and leak runs;
- doctor and diagnose modes for required tools, target file layout, header
  guards, `get_next_line` prototypes, header compile probes, Makefile shape
  when present, and lightweight norm-style warnings;
- compare mode for two target roots;
- internal self-test fixtures for known-good and known-broken targets.

## Not Yet Covered

Planned future coverage:

- 1MB line fixtures;
- alternating large-line fixtures;
