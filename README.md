# Get Next Line Tester

Standalone tester for 42-style `get_next_line` projects.

It provides a small command-line driver, `--root` based target selection,
multiple `BUFFER_SIZE` profiles, mandatory checks, bonus checks, review
summaries, optional stress fixtures, and optional Valgrind leak checks.

## Documentation

| File | Contents |
| --- | --- |
| [`CHANGELOG.md`](CHANGELOG.md) | Version history and planned changes. |
| [`docs/USAGE.md`](docs/USAGE.md) | Command and workflow guide. |
| [`docs/COVERAGE.md`](docs/COVERAGE.md) | Current test coverage. |
| [`docs/TROUBLESHOOTING.md`](docs/TROUBLESHOOTING.md) | Common failures and fixes. |
| [`CONTRIBUTING.md`](CONTRIBUTING.md) | Contribution guidelines. |
| [`SECURITY.md`](SECURITY.md) | Security policy. |

## Quick Start

```sh
make build
./gnl_tester --root /path/to/Get_Next_Line
```

Or with `make`:

```sh
make ROOT_DIR=/path/to/Get_Next_Line
```

Example with an absolute target path:

```sh
./gnl_tester --root /absolute/path/to/Get_Next_Line
```

## Options

```text
--root PATH          target get_next_line project directory (default: ..)
--bonus             use get_next_line_bonus.c/.h and test interleaved fds
--review            run mandatory strict and bonus strict when available
--stress            enable large-line stress fixtures
--buffer LIST       comma-separated BUFFER_SIZE list
--quick             BUFFER_SIZE=1,42
--strict            BUFFER_SIZE=1,2,3,4,5,7,8,16,32,42,64,128,1024
--leaks             run each suite with Valgrind when available
--timeout MS        kill a test run after this many ms (default: 3000)
--no-color          disable colors
--help              show help
```

## What It Checks

- invalid fd;
- empty file;
- line without a final newline;
- line with a final newline;
- multiple lines;
- consecutive empty lines;
- lines larger than `BUFFER_SIZE`;
- pipe file descriptors;
- opt-in 10k and 100k line stress fixtures;
- buffer boundary cases;
- repeated calls after EOF;
- test runs that exceed the configured timeout;
- review summaries for pre-submission checks;
- bonus mode with interleaved and many file descriptor checks.

The tester compiles the target project once per `BUFFER_SIZE` with:

```text
cc -Wall -Wextra -Werror -D BUFFER_SIZE=N
```

This catches bugs that only appear with tiny buffers, larger buffers, or
boundary-sized reads.
