# Get Next Line Tester

![Language](https://img.shields.io/badge/language-C%2B%2B17-00599C)
![Project](https://img.shields.io/badge/project-get_next_line-111111)
![Mode](https://img.shields.io/badge/modes-mandatory%20%7C%20bonus-informational)
![Release](https://img.shields.io/badge/release-v0.3.0-blue)
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

- README reworked into a fuller public project guide.
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
- pipe file descriptors;
- repeated calls after EOF;
- test runs that exceed the configured timeout;
- optional 10k and 100k line stress fixtures;
- bonus interleaved reads across multiple file descriptors;
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
./gnl_tester --root ../Get_Next_Line --quick
```

If that passes, run the strict mandatory matrix:

```sh
./gnl_tester --root ../Get_Next_Line --strict
```

If your project includes bonus files, run:

```sh
./gnl_tester --root ../Get_Next_Line --bonus --strict
```

Before sharing or submitting, run the compact review flow:

```sh
./gnl_tester --root ../Get_Next_Line --review
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
./gnl_tester --root ../Get_Next_Line --no-color
./gnl_tester --help
```

Use `--quick` while coding, `--strict` before sharing, `--review` when you want
a compact pre-submission verdict, and `--stress` when you want to exercise large
line handling.

## Minimal Makefile Commands

The Makefile is intentionally small:

| Command | Purpose |
| --- | --- |
| `make build` | Build the standalone `./gnl_tester` driver. |
| `make ROOT_DIR=../Get_Next_Line` | Build, then run the tester as a shortcut. |
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

The current documented release is `v0.3.0`. Active development is tracked under
`Unreleased` in [`CHANGELOG.md`](CHANGELOG.md), and the next release checklist
is kept in [`docs/RELEASE_PLAN.md`](docs/RELEASE_PLAN.md).

Before publishing a release, run:

```sh
make re
./gnl_tester --root /path/to/Get_Next_Line --strict --no-color
./gnl_tester --root /path/to/Get_Next_Line --bonus --strict --no-color
./gnl_tester --root /path/to/Get_Next_Line --review --no-color
```

## Documentation

| Document | What it explains |
| --- | --- |
| [Usage guide](docs/USAGE.md) | CLI commands and workflows. |
| [Coverage table](docs/COVERAGE.md) | Tested mandatory, bonus, stress, and buffer cases. |
| [Troubleshooting](docs/TROUBLESHOOTING.md) | Common setup and failure fixes. |
| [Contributing tests](docs/CONTRIBUTING_TESTS.md) | How to add reliable tests. |
| [Release plan](docs/RELEASE_PLAN.md) | Release checklist and next-version scope. |
| [Release notes v0.3.0](docs/RELEASE_NOTES_v0.3.0.md) | Current release notes. |
| [Release notes v0.2.0](docs/RELEASE_NOTES_v0.2.0.md) | Previous release notes. |
| [Changelog](CHANGELOG.md) | Project history and releases. |
| [Contributing guide](CONTRIBUTING.md) | How to contribute to the tester. |
| [Security policy](SECURITY.md) | Supported security reporting process. |

## Contributing

Contributions are welcome. If you want to add tests, fix docs, or improve the
tester, start with:

- [Contributing guide](CONTRIBUTING.md)
- [Contributing tests](docs/CONTRIBUTING_TESTS.md)
- [Coverage table](docs/COVERAGE.md)

## License

This project is released under the [MIT License](LICENSE).
