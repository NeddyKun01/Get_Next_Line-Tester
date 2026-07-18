# Get Next Line Tester

![Language](https://img.shields.io/badge/language-C%2B%2B17-00599C)
![Project](https://img.shields.io/badge/project-get_next_line-111111)
![Mode](https://img.shields.io/badge/modes-mandatory%20%7C%20bonus-informational)
[![CI](https://github.com/NeddyKun01/Get_Next_Line-Tester/actions/workflows/ci.yml/badge.svg)](https://github.com/NeddyKun01/Get_Next_Line-Tester/actions/workflows/ci.yml)
![Reports](https://img.shields.io/badge/reports-terminal%20%7C%20JSON%20%7C%20Web-2ea44f)
![Release](https://img.shields.io/badge/release-v1.0.0-blue)
![License](https://img.shields.io/badge/license-MIT-green)

Get Next Line Tester is a standalone, terminal-friendly tester for
`get_next_line` projects.

It is useful for students, reviewers, maintainers, and anyone working on a
42-style line reader that must behave correctly across different
`BUFFER_SIZE` values. The tester focuses on mandatory behavior, bonus
multi-file-descriptor behavior, timeout protection, optional stress fixtures,
and optional Valgrind leak checks.

The main command is intentionally simple:

```sh
./gnl_tester --root ../Get_Next_Line
```

If the driver is not built yet, run `make build` once first. The `make` command
also works as a convenience shortcut, but the day-to-day interface is the
standalone `./gnl_tester` binary.

## What's New

- `--web` and `--html` for standalone Web dashboard reports.
- `--doctor`, `--diagnose`, and `--health` for local tools, target layout,
  Makefile, header integrity, and header compile probes.
- `--doctor --web` and `--doctor --json` for shareable diagnostics.
- `--cases`, `--explain CASE`, `--only CASE`, and `--skip CASE` for focused
  debugging with validated case names.
- `--export-fixture CASE` and `--rerun-failed FILE` for tighter debug loops.
- `--compact`, `--profile`, `--fd-limit`, `--norm`, and `--malloc-fail` for
  automation, performance, bonus, style, and allocation-failure checks.
- `--preset pedantic` for a heavier local matrix while keeping `school` as a
  strict preset.
- `--list`, `--coverage`, and `--coverage-md` for inspecting tester coverage
  directly from the binary.
- `--compare PATH` for comparing two Get Next Line roots with the same options.
- `--preset NAME` and `--presets` for repeatable quick, review, CI, Web,
  stress, and leak workflows.
- `--output FILE` for writing JSON and Web reports without shell redirection.
- Optional `.gnl-tester.json` defaults for local preset, root, timeout, and
  output preferences.
- Build directories are isolated per process so parallel tester runs do not
  overwrite each other.
- Build directories are cleaned automatically unless `--keep-build` is used.
- `make self-test` validates the tester against known-good and known-broken
  fixtures.
- Mandatory mode now checks `STDIN_FILENO` in addition to regular files and
  pipe file descriptors.
- Reports include per-buffer duration metadata.
- Web reports include likely-fix hints for failing rows.
- GitHub Actions now uploads a Web dashboard artifact for target-project runs.
- `--json` for machine-readable CI and script output.
- `--summary-only` for cleaner CI-style output.
- `--fail-fast` to stop after the first failing buffer suite.
- GitHub Actions for tester smoke checks and optional target-project CI.
- Custom issue templates for bug reports, feature requests, and questions.
- Clearer first-run workflow for quick, strict, bonus, and review runs.
- Result statuses, buffer profiles, advanced CLI examples, Makefile commands,
  leak checks, release status, documentation links, and contributing entry
  points are now documented from the main page.
- Release notes and release planning remain available for versioned publishing.

## What It Checks

The tester covers:

- invalid file descriptors;
- empty files;
- one line without a final newline;
- one line with a final newline;
- multiple lines;
- consecutive empty lines;
- lines larger than `BUFFER_SIZE`;
- buffer boundary cases;
- exact `BUFFER_SIZE - 1`, `BUFFER_SIZE`, and `BUFFER_SIZE + 1` line cases;
- exact `BUFFER_SIZE * 2` and `BUFFER_SIZE * 2 + 1` line cases;
- newline-only files and many-newline files;
- invalid read-path repeat checks;
- pipe file descriptors;
- stdin-backed reads;
- closed file descriptors;
- repeated calls after EOF;
- test runs that exceed the configured timeout;
- optional 10k and 100k line stress fixtures;
- bonus interleaved reads across multiple file descriptors;
- bonus wide multi-fd round-robin checks;
- optional Valgrind leak checks.

The tester compiles the target project once per selected `BUFFER_SIZE` with:

```text
cc -Wall -Wextra -Werror -D BUFFER_SIZE=N
```

This catches bugs that only appear with tiny buffers, larger buffers, or
boundary-sized reads.

## Requirements

You need:

| Tool | Why |
| --- | --- |
| `make` | Builds the tester through the repository Makefile. |
| `c++` | Compiles the C++17 driver. |
| `cc` | Compiles the target C files and harnesses. |
| `valgrind` | Optional, only needed for leak checks. |

Mandatory mode expects the target project to contain:

```text
Get_Next_Line/
├── get_next_line.c
├── get_next_line_utils.c
└── get_next_line.h
```

Bonus mode expects:

```text
Get_Next_Line/
├── get_next_line_bonus.c
├── get_next_line_utils_bonus.c
└── get_next_line_bonus.h
```

## Why No Shell Scripts?

The tester is intentionally driven by C++ instead of shell helper scripts.
That makes it friendlier on school, shared, or restricted machines where script
execution permissions can be annoying.

`./gnl_tester` is the standalone driver. It validates the target layout,
compiles the selected harnesses, runs each `BUFFER_SIZE` suite, handles
timeouts, and reports functional or leak-check results.

## Quick Start

Clone this tester next to your target project:

```text
projects/
├── Get_Next_Line/
└── Get_Next_Line-Tester/
```

Run:

```sh
cd Get_Next_Line-Tester
make build
./gnl_tester --root ../Get_Next_Line
```

If the tester is inside your `Get_Next_Line` repository:

```text
Get_Next_Line/
├── get_next_line.c
├── get_next_line_utils.c
├── get_next_line.h
└── tester/
```

Run:

```sh
cd tester
make build
./gnl_tester --root ..
```

If your target project is somewhere else, pass an absolute path:

```sh
./gnl_tester --root /absolute/path/to/Get_Next_Line
```

## First Run Recommendation

Start with a quick mandatory run:

```sh
./gnl_tester --root ../Get_Next_Line --preset quick
```

If that passes, run the strict mandatory matrix:

```sh
./gnl_tester --root ../Get_Next_Line --preset strict
```

If your project includes bonus files, run:

```sh
./gnl_tester --root ../Get_Next_Line --bonus --strict
```

Before sharing or submitting, run the compact review flow:

```sh
./gnl_tester --root ../Get_Next_Line --preset review
```

## Understanding Results

| Status | Meaning |
| --- | --- |
| `OK` | The suite passed for that `BUFFER_SIZE`. |
| `NOK` | The suite failed for that `BUFFER_SIZE`. |
| `timeout(...)` | The compiled test run exceeded the configured timeout. |
| `Valgrind: OK` | Leak checks passed. |
| `Valgrind: NOK ...` | Valgrind found leaks or memory errors. |
| `Valgrind: SKIP` | Valgrind is not installed or leak checks were not requested. |

Example mandatory output:

```text
Get Next Line Tester
target: "../Get_Next_Line"
mode:   mandatory

OK  BUFFER_SIZE=1
OK  BUFFER_SIZE=42

Summary: 2/2 buffer suites passed
```

Example review output:

```text
Get Next Line Tester Review

target:  "../Get_Next_Line"
timeout: 3000ms
stress:  skipped
leaks:   skipped

Mandatory: OK 13/13
Bonus:     OK 13/13
Valgrind: SKIP
Verdict:  PASS
```

When a line comparison fails, the harness prints the failing label and compact
expected/got values so the broken case is visible in the terminal.

## Buffer Profiles

| Profile | Values |
| --- | --- |
| `--quick` | `1,42` |
| default | `1,2,3,5,8,16,32,42,128,1024` |
| `--strict` | `1,2,3,4,5,7,8,16,32,42,64,128,1024` |
| `--buffer LIST` | Custom comma-separated values. |

Examples:

```sh
./gnl_tester --root ../Get_Next_Line --quick
./gnl_tester --root ../Get_Next_Line --strict
./gnl_tester --root ../Get_Next_Line --buffer 1,2,42,1024
```

## Advanced CLI

Build the tester driver once:

```sh
make build
```

Then run direct commands:

```sh
./gnl_tester --root ../Get_Next_Line
./gnl_tester --root ../Get_Next_Line --quick
./gnl_tester --root ../Get_Next_Line --strict
./gnl_tester --root ../Get_Next_Line --buffer 1,42,1024
./gnl_tester --root ../Get_Next_Line --bonus
./gnl_tester --root ../Get_Next_Line --bonus --strict
./gnl_tester --root ../Get_Next_Line --stress --buffer 42 --timeout 10000
./gnl_tester --root ../Get_Next_Line --leaks
./gnl_tester --root ../Get_Next_Line --review
./gnl_tester --root ../Get_Next_Line --review --leaks
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
./gnl_tester --root ../Get_Next_Line --compare ../Get_Next_Line-before --quick
./gnl_tester --root ../Get_Next_Line --no-color
./gnl_tester --help
```

Use `--preset quick` while coding, `--preset strict` before sharing,
`--preset review` when you want a compact pre-submission verdict, and
`--preset stress` when you want to exercise large line handling. Use
`--preset pedantic` for a heavier local matrix with stress enabled.

## Command Finder

| I want to... | Command |
| --- | --- |
| check my setup | `./gnl_tester --root ../Get_Next_Line --doctor` |
| get machine-readable diagnostics | `./gnl_tester --root ../Get_Next_Line --doctor --json` |
| share diagnostics as HTML | `./gnl_tester --root ../Get_Next_Line --doctor --web --output gnl-doctor.html` |
| see valid focused cases | `./gnl_tester --cases` |
| understand one case | `./gnl_tester --explain buffer-edge` |
| debug one case | `./gnl_tester --root ../Get_Next_Line --only stdin --quick` |
| export a case fixture | `./gnl_tester --export-fixture double-buffer --buffer 42 --output fixtures` |
| rerun previous failures | `./gnl_tester --root ../Get_Next_Line --rerun-failed gnl-test-report.json` |
| check allocation failure | `./gnl_tester --root ../Get_Next_Line --only malloc-fail --quick` |
| profile slow buffers | `./gnl_tester --root ../Get_Next_Line --quick --profile` |
| skip a noisy case | `./gnl_tester --root ../Get_Next_Line --skip buffer-edge --strict` |
| run before submission | `./gnl_tester --root ../Get_Next_Line --preset review` |
| generate CI JSON | `./gnl_tester --root ../Get_Next_Line --preset ci --output gnl-test-report.json` |
| generate a Web report | `./gnl_tester --root ../Get_Next_Line --preset web --output gnl-test-report.html` |
| run the heaviest local preset | `./gnl_tester --root ../Get_Next_Line --preset pedantic` |

Use `--doctor` when a target does not compile or the first run feels suspicious.
It checks required tools, local harness files, target files, header guards,
`get_next_line` prototypes, header compile probes, Makefile shape when present,
and prints a recommended next command. Use `--diagnose` for only the target
project checks, or `--health` for a single-line status summary.

Use `--explain CASE` to inspect what a case is trying to catch, then rerun a
focused subset with `--only CASE` or `--skip CASE`:

```sh
./gnl_tester --cases
./gnl_tester --explain buffer-edge
./gnl_tester --root ../Get_Next_Line --only buffer-edge --strict
```

Use `--list`, `--coverage`, or `--coverage-md` when you want to inspect the
tester itself without opening documentation files.

Use `--compare PATH` to run the same selected mode against another target root:

```sh
./gnl_tester --root ../Get_Next_Line --compare ../Get_Next_Line-before --quick
```

Use `--summary-only` for cleaner automation logs. Use `--fail-fast` when you
want the first failing `BUFFER_SIZE` to stop the run immediately.

Use `--json` when a script or CI job needs structured output. JSON mode disables
color automatically and writes only JSON to stdout. Run results include
diagnostics, case filters, failed case labels, and likely-fix hints.

Use `--output FILE` with JSON or Web output when you want the tester to write
the report directly:

```sh
./gnl_tester --root ../Get_Next_Line --preset ci --output gnl-test-report.json
```

Use `--web` or `--html` when you want a standalone dashboard report:

```sh
./gnl_tester --root ../Get_Next_Line --review --web > gnl-test-report.html
```

Web mode disables color automatically, keeps stdout limited to HTML, and
includes the run configuration, diagnostics, suite scores, buffer results,
failure details, likely-fix hints, filters, durations, command metadata, and
rerun commands.

Optional local defaults can be stored in `.gnl-tester.json`:

```json
{
  "root": "../Get_Next_Line",
  "preset": "review",
  "timeout_ms": 5000,
  "summary_only": true
}
```

CLI flags always override config-file values.

## Minimal Makefile Commands

The Makefile is intentionally small:

| Command | Purpose |
| --- | --- |
| `make build` | Build the standalone `./gnl_tester` driver. |
| `make ROOT_DIR=../Get_Next_Line` | Build, then run the tester as a shortcut. |
| `make self-test` | Validate the tester against internal good and broken fixtures. |
| `make clean-runs` | Remove per-run build directories without deleting the binary. |
| `make clean` | Remove tester build files. |
| `make fclean` | Remove build files and the tester binary. |
| `make re` | Rebuild the driver. |

Everything else is available through `./gnl_tester`.

## Leak Checks

`--leaks` runs each suite through Valgrind when Valgrind is available:

```sh
./gnl_tester --root ../Get_Next_Line --leaks
./gnl_tester --root ../Get_Next_Line --bonus --strict --leaks
```

The tester classifies common Valgrind failures, including definitely lost,
indirectly lost, possibly lost, still reachable, invalid read, invalid write,
invalid free, and uninitialised value reports.

## Release Status

The current release is `v1.0.0`. Release history is tracked in
[`CHANGELOG.md`](CHANGELOG.md), and the release checklist is kept in
[`docs/RELEASE_PLAN.md`](docs/RELEASE_PLAN.md).

Before publishing a release, run:

```sh
make re
./gnl_tester --root /path/to/Get_Next_Line --doctor
./gnl_tester --root /path/to/Get_Next_Line --diagnose
./gnl_tester --root /path/to/Get_Next_Line --health
./gnl_tester --root /path/to/Get_Next_Line --strict --no-color
./gnl_tester --root /path/to/Get_Next_Line --bonus --strict --no-color
./gnl_tester --root /path/to/Get_Next_Line --review --no-color
./gnl_tester --root /path/to/Get_Next_Line --preset ci --output /tmp/gnl-test-report.json
./gnl_tester --root /path/to/Get_Next_Line --preset web --output /tmp/gnl-test-report.html
```

## GitHub Actions

This repository includes a workflow for tester smoke checks and optional
target-project checks.

Set this repository variable to run target checks automatically:

```text
GNL_REPOSITORY=owner/repository
```

Example:

```text
GNL_REPOSITORY=OWNER/Get_Next_Line
```

The workflow builds the tester, runs a known-good smoke fixture, validates
doctor mode, validates Web report generation through `--output`, and can run
mandatory strict, bonus strict, review, and Valgrind checks against the
configured external Get Next Line repository. It also uploads
`gnl-test-report.json` as a machine-readable artifact and `gnl-test-report.html`
as a standalone Web dashboard artifact.

## Documentation

| Document | What it explains |
| --- | --- |
| [Usage guide](docs/USAGE.md) | CLI commands and workflows. |
| [Coverage table](docs/COVERAGE.md) | Tested mandatory, bonus, stress, and buffer cases. |
| [Test cases](docs/TEST_CASES.md) | Focused case names, inputs, expected behavior, and common failures. |
| [Troubleshooting](docs/TROUBLESHOOTING.md) | Common setup and failure fixes. |
| [GitHub Actions guide](docs/GITHUB_ACTIONS.md) | CI variables, artifacts, and reusable workflow examples. |
| [Contributing tests](docs/CONTRIBUTING_TESTS.md) | How to add reliable tests. |
| [Release plan](docs/RELEASE_PLAN.md) | Release checklist and next-version scope. |
| [Release notes v1.0.0](docs/RELEASE_NOTES_v1.0.0.md) | Current release notes. |
| [Release notes v0.7.0](docs/RELEASE_NOTES_v0.7.0.md) | Previous release notes. |
| [Release notes v0.6.0](docs/RELEASE_NOTES_v0.6.0.md) | Previous release notes. |
| [Release notes v0.5.0](docs/RELEASE_NOTES_v0.5.0.md) | Previous release notes. |
| [Release notes v0.4.0](docs/RELEASE_NOTES_v0.4.0.md) | Earlier release notes. |
| [Release notes v0.3.0](docs/RELEASE_NOTES_v0.3.0.md) | Earlier release notes. |
| [Release notes v0.2.0](docs/RELEASE_NOTES_v0.2.0.md) | Earlier release notes. |
| [Changelog](CHANGELOG.md) | Project history and releases. |
| [Contributing guide](CONTRIBUTING.md) | How to contribute to the tester. |
| [Security policy](SECURITY.md) | Supported security reporting process. |

## Contributing

Contributions are welcome. If you want to add tests, fix docs, or improve the
tester, start with:

- [Contributing guide](CONTRIBUTING.md)
- [Contributing tests](docs/CONTRIBUTING_TESTS.md)
- [Coverage table](docs/COVERAGE.md)

The repository also includes issue templates for bug reports, feature requests,
and usage questions under `.github/ISSUE_TEMPLATE/`.

## License

This project is released under the [MIT License](LICENSE).
