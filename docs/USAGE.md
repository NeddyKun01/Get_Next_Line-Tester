# Usage Guide

Get Next Line Tester has two workflows:

- use `./gnl_tester` for direct commands, scripts, and CI;
- use `make` as a convenience wrapper when you want it.

## Project Layouts

The tester can live next to your target repository:

```text
projects/
├── Get_Next_Line/
└── Get_Next_Line-Tester/
```

```sh
cd Get_Next_Line-Tester
make build
./gnl_tester --root ../Get_Next_Line
```

The tester can also live anywhere else when you pass an absolute path:

```sh
./gnl_tester --root /absolute/path/to/Get_Next_Line
```

## Minimal Make Commands

| Command | Purpose |
| --- | --- |
| `make build` | Build the standalone `./gnl_tester` driver. |
| `make ROOT_DIR=../Get_Next_Line` | Build and run the default suite. |
| `make ROOT_DIR=../Get_Next_Line doctor` | Run doctor mode. |
| `make presets` | List named presets. |
| `make ROOT_DIR=../Get_Next_Line review` | Run the review preset. |
| `make ROOT_DIR=../Get_Next_Line report-json` | Write `gnl-test-report.json`. |
| `make ROOT_DIR=../Get_Next_Line report-web` | Write `gnl-test-report.html`. |
| `make self-test` | Validate the tester against internal good and broken fixtures. |
| `make clean-runs` | Remove per-run build directories without deleting the binary. |
| `make clean` | Remove tester build files and reports. |
| `make fclean` | Same as `clean`. |
| `make re` | Rebuild the driver. |

## Direct CLI Workflow

Build the driver first:

```sh
make build
```

Then run the binary:

```sh
./gnl_tester --root ../Get_Next_Line
./gnl_tester --root ../Get_Next_Line --quick
./gnl_tester --root ../Get_Next_Line --strict
./gnl_tester --root ../Get_Next_Line --bonus --strict
./gnl_tester --root ../Get_Next_Line --review
./gnl_tester --root ../Get_Next_Line --stress --buffer 42 --timeout 10000
./gnl_tester --root ../Get_Next_Line --buffer 1,2,42,1024
./gnl_tester --root ../Get_Next_Line --strict --timeout 5000
./gnl_tester --root ../Get_Next_Line --strict --leaks --no-color
./gnl_tester --root ../Get_Next_Line --strict --summary-only --no-color
./gnl_tester --root ../Get_Next_Line --strict --fail-fast
./gnl_tester --root ../Get_Next_Line --review --json
./gnl_tester --root ../Get_Next_Line --review --web > gnl-test-report.html
./gnl_tester --root ../Get_Next_Line --preset ci --output gnl-test-report.json
./gnl_tester --root ../Get_Next_Line --preset web --output gnl-test-report.html
./gnl_tester --root ../Get_Next_Line --doctor
./gnl_tester --root ../Get_Next_Line --diagnose
./gnl_tester --root ../Get_Next_Line --health
./gnl_tester --root ../Get_Next_Line --doctor --json
./gnl_tester --root ../Get_Next_Line --doctor --web --output gnl-doctor.html
./gnl_tester --cases
./gnl_tester --explain stdin
./gnl_tester --root ../Get_Next_Line --only stdin --quick
./gnl_tester --root ../Get_Next_Line --skip buffer-edge --strict
./gnl_tester --root ../Get_Next_Line --only malloc-fail --quick
./gnl_tester --root ../Get_Next_Line --bonus --only bonus-wide-fds --fd-limit 64
./gnl_tester --export-fixture double-buffer --buffer 42 --output exported-fixtures
./gnl_tester --root ../Get_Next_Line --rerun-failed gnl-test-report.json
./gnl_tester --root ../Get_Next_Line --quick --profile
./gnl_tester --root ../Get_Next_Line --review --compact
./gnl_tester --root ../Get_Next_Line --preset pedantic
./gnl_tester --presets
./gnl_tester --list
./gnl_tester --coverage
./gnl_tester --coverage-md
./gnl_tester --root ../Get_Next_Line --compare ../Get_Next_Line-before --quick
./gnl_tester --help
```

## Diagnostics

Use `--doctor` when a target does not compile, when a path looks suspicious, or
before opening an issue:

```sh
./gnl_tester --root ../Get_Next_Line --doctor
```

Doctor mode checks required tools, local tester harness files, mandatory target
files, bonus target files, header guards, `get_next_line` prototypes, header
compile probes, optional Valgrind availability, and Makefile shape when a target
Makefile exists. It also prints a recommended next command.

Use `--diagnose` when you only want target-project checks:

```sh
./gnl_tester --root ../Get_Next_Line --diagnose
```

Use `--health` for a compact single-line status:

```sh
./gnl_tester --root ../Get_Next_Line --health
```

Use `--doctor --json` when automation needs structured diagnostics:

```sh
./gnl_tester --root ../Get_Next_Line --doctor --json
```

Use `--doctor --web` for a shareable HTML diagnostic report without running the
test suites:

```sh
./gnl_tester --root ../Get_Next_Line --doctor --web --output gnl-doctor.html
```

## Presets

Use `--presets` to list named workflows:

```sh
./gnl_tester --presets
```

Available presets:

| Preset | Best for |
| --- | --- |
| `quick` | Fast feedback while editing. |
| `normal` | Default balanced run. |
| `strict` | Stronger local validation. |
| `school` | Alias for strict pre-submission checks. |
| `review` | Compact reviewer-friendly mandatory and bonus flow. |
| `ci` | JSON review report with CI-friendly defaults. |
| `web` | Standalone Web review dashboard. |
| `html` | Alias for the Web dashboard preset. |
| `stress` | Large-line mandatory fixtures. |
| `leaks` | Focused Valgrind leak checks. |
| `pedantic` | Heavier local matrix with stress enabled. |

## List And Coverage

Use `--list` to print available suites, presets, and report types:

```sh
./gnl_tester --list
```

Use `--coverage` or `--coverage-md` to inspect tested behavior:

```sh
./gnl_tester --coverage
./gnl_tester --coverage-md
```

Use `--explain CASE` to print a short description of a specific case:

```sh
./gnl_tester --cases
./gnl_tester --explain stdin
./gnl_tester --explain buffer-edge
```

Focused runs can include or exclude a named case:

```sh
./gnl_tester --root ../Get_Next_Line --only stdin --quick
./gnl_tester --root ../Get_Next_Line --skip buffer-edge --strict
./gnl_tester --root ../Get_Next_Line --only malloc-fail --quick
```

Use `--export-fixture CASE` to write representative input files for manual
debugging:

```sh
./gnl_tester --export-fixture double-buffer --buffer 42 --output exported-fixtures
```

Use `--rerun-failed FILE` to rerun only the failed buffers from a previous JSON
report:

```sh
./gnl_tester --root ../Get_Next_Line --rerun-failed gnl-test-report.json
```

## Config File

Optional local defaults can be stored in `.gnl-tester.json`:

```json
{
  "root": "../Get_Next_Line",
  "preset": "review",
  "timeout_ms": 5000,
  "summary_only": true,
  "keep_build": false
}
```

Supported keys include `root`, `output`, `preset`, `buffers`, `timeout_ms`,
`bonus`, `review`, `stress`, `leaks`, `summary_only`, `fail_fast`,
`keep_build`, and `color`. CLI flags always override config-file values.

## Buffer Profiles

| Option | Buffer sizes | Best for |
| --- | --- | --- |
| default | `1,2,3,5,8,16,32,42,128,1024` | Normal local validation. |
| `--quick` | `1,42` | Fast feedback while editing. |
| `--strict` | `1,2,3,4,5,7,8,16,32,42,64,128,1024` | Stronger pre-review checks. |
| `--buffer LIST` | custom comma-separated list | Focused debugging. |

The tester recompiles the target once per `BUFFER_SIZE`:

```text
cc -Wall -Wextra -Werror -D BUFFER_SIZE=N
```

This catches bugs that only appear with tiny buffers, larger buffers, or exact
boundary lengths.

Mandatory mode also includes pipe-backed and stdin-backed file descriptor cases.
This checks that `get_next_line` works with valid non-regular-file descriptors,
not only with files opened from disk.

## Timeout

Each compiled test run is killed if it exceeds the configured timeout. The
default is `3000` milliseconds.

Use a larger timeout for slow machines or heavy Valgrind runs:

```sh
./gnl_tester --root ../Get_Next_Line --strict --timeout 5000
```

Use `--timeout 0` to disable the timeout.

## Output Controls

Use `--summary-only` for cleaner automation logs:

```sh
./gnl_tester --root ../Get_Next_Line --strict --summary-only --no-color
```

In normal mode, passing buffer suites are hidden and failing buffer suites still
print their useful diagnostics. In review mode, the output stays limited to the
compact review summary.

Use `--fail-fast` to stop after the first failing buffer suite:

```sh
./gnl_tester --root ../Get_Next_Line --strict --fail-fast
```

This is useful when a tiny `BUFFER_SIZE` already exposes the bug and later
buffers would only add noise.

Use `--compact` for a single-line verdict:

```sh
./gnl_tester --root ../Get_Next_Line --review --compact
```

Use `--profile` to print the slowest buffer suites after a terminal run:

```sh
./gnl_tester --root ../Get_Next_Line --quick --profile
```

Every run uses a unique build directory under `tester/build`, so two independent
tester processes can run at the same time without overwriting each other's
compile outputs, logs, or fixture files. By default, these per-run directories
are cleaned after the run finishes. Use `--keep-build` when you need to inspect
the generated harness logs and fixture files.

## JSON Output

Use `--json` when CI, scripts, or bots need structured output:

```sh
./gnl_tester --root ../Get_Next_Line --review --json
./gnl_tester --root ../Get_Next_Line --strict --json
```

JSON mode disables color automatically and writes only JSON to stdout. It
includes the target path, mode, selected buffers, pass/fail summary, case
filters, diagnostics, per-buffer results, failed case labels, likely-fix hints,
timeout state, and Valgrind issue categories when leak checks are enabled.

Use `--output FILE` when you want the tester to write the JSON report directly:

```sh
./gnl_tester --root ../Get_Next_Line --preset ci --output gnl-test-report.json
```

## Web Dashboard

Use `--web` or `--html` when you want a standalone dashboard report:

```sh
./gnl_tester --root ../Get_Next_Line --review --web > gnl-test-report.html
./gnl_tester --root ../Get_Next_Line --strict --html > gnl-test-report.html
```

Web mode disables color automatically and writes only HTML to stdout. The report
is self-contained and includes:

- target path and run configuration;
- diagnostics for headers, probes, Makefile, and warnings;
- mandatory and bonus suite summaries;
- per-buffer pass/fail results;
- failure details for compile, test, timeout, and Valgrind errors;
- likely-fix hints for failing rows;
- filters for all, passed, and failed rows;
- per-buffer duration metadata;
- the original command line;
- copyable rerun commands for failing buffer suites.

Use `--output FILE` when you want the tester to write the Web report directly:

```sh
./gnl_tester --root ../Get_Next_Line --preset web --output gnl-test-report.html
```

## Stress Mode

Use `--stress` to enable large-line fixtures. Stress mode currently adds 10k
and 100k line cases to mandatory tests.

```sh
./gnl_tester --root ../Get_Next_Line --stress --buffer 42 --timeout 10000
```

Stress mode can be intentionally heavy with very small buffers. If you combine
`--stress` with `BUFFER_SIZE=1`, consider raising `--timeout`.

## Review Mode

Use `--review` for a compact pre-submission check:

```sh
./gnl_tester --root ../Get_Next_Line --review
```

Review mode runs the strict mandatory matrix and automatically runs the strict
bonus matrix when bonus files are present. It prints a compact summary and a
final `PASS` or `FAIL` verdict.

Use `--leaks` with review mode when you also want Valgrind checks:

```sh
./gnl_tester --root ../Get_Next_Line --review --leaks
```

Use `--stress` with review mode only when you want the heavier large-line cases
included in the mandatory matrix:

```sh
./gnl_tester --root ../Get_Next_Line --review --stress --timeout 10000
```

## Compare Mode

Use `--compare PATH` to run the same selected mode against another target root:

```sh
./gnl_tester --root ../Get_Next_Line --compare ../Get_Next_Line-before --quick
./gnl_tester --root ../Get_Next_Line --compare ../Get_Next_Line-before --bonus --strict
```

Compare mode reports both scores, both durations, and whether the target score
is better, worse, or the same.

## Bonus Mode

Use `--bonus` when the target repository includes:

```text
get_next_line_bonus.c
get_next_line_utils_bonus.c
get_next_line_bonus.h
```

Example:

```sh
./gnl_tester --root ../Get_Next_Line --bonus --strict
```

Bonus mode checks that two file descriptors can be read in an interleaved order
without their internal state leaking into each other.

Use `--fd-limit N` with `bonus-wide-fds` to choose the wide fd count:

```sh
./gnl_tester --root ../Get_Next_Line --bonus --only bonus-wide-fds --fd-limit 64
```

## Norminette

Use `--norm` with diagnostics when you want optional Norminette feedback:

```sh
./gnl_tester --root ../Get_Next_Line --diagnose --norm
```

If `norminette` is not installed, diagnostics report that it was skipped.

## Leak Checks

Use `--leaks` to run each compiled suite under Valgrind when it is installed:

```sh
./gnl_tester --root ../Get_Next_Line --strict --leaks --no-color
```

When Valgrind fails, the tester prints a short category before the full log:

```text
Valgrind: NOK invalid read, definitely lost
```

If Valgrind is missing, the tester skips leak checks and prints a note.

## GitHub Actions

The repository includes a GitHub Actions workflow at
`.github/workflows/ci.yml`.

The workflow always builds the tester and runs a smoke suite against a
known-good fixture. To run checks against an external Get Next Line repository,
set this repository variable:

```text
GNL_REPOSITORY=owner/repository
```

You can also start the workflow manually and provide `gnl_repository` through
`workflow_dispatch`.

When a target repository is configured, the workflow runs:

- tester smoke checks, including doctor mode, preset listing, and Web report
  generation against a known-good fixture;
- `make self-test` against internal known-good and known-broken fixtures;
- mandatory strict mode with `--summary-only --fail-fast`;
- bonus strict mode when bonus files are present;
- review mode with an uploaded `gnl-review.txt` artifact;
- JSON review mode with an uploaded `gnl-test-report.json` artifact;
- Web review mode with an uploaded `gnl-test-report.html` artifact;
- a focused Valgrind leak job.

See [GitHub Actions guide](GITHUB_ACTIONS.md) for reusable examples and artifact
details.
