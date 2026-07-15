# Contributing

Thank you for helping improve Get Next Line Tester.

The project should stay small, predictable, and useful for anyone validating a
42-style `get_next_line` implementation locally or during review.

## Before You Start

Keep each contribution focused on one clear goal:

- bug fix;
- new test coverage;
- documentation improvement;
- output or usability improvement;
- CI or tooling improvement.

If a change makes the tester stricter, document the new expectation and prefer
adding a focused fixture over adding many similar cases.

## Development Setup

Use a local target project while developing the tester:

```sh
make build
./gnl_tester --root /path/to/Get_Next_Line --strict
./gnl_tester --root /path/to/Get_Next_Line --bonus --strict
```

For faster iteration:

```sh
./gnl_tester --root /path/to/Get_Next_Line --quick
```

If your change touches allocation or fd cleanup behavior, also run:

```sh
./gnl_tester --root /path/to/Get_Next_Line --strict --leaks --no-color
```

## Adding Tests

The current tester generates a temporary C harness from
`tester/src/main.cpp`, compiles it against the target source files, and runs it
once per selected `BUFFER_SIZE`.

When adding coverage, prefer cases that prove different behavior:

- empty input;
- no final newline;
- multiple consecutive newlines;
- line length smaller than `BUFFER_SIZE`;
- line length exactly at a buffer boundary;
- line length larger than `BUFFER_SIZE`;
- repeated calls after EOF;
- invalid file descriptors;
- interleaved fds for bonus mode.

Avoid tests that require undefined C behavior. The default suite should focus on
behavior expected by the project subject and normal POSIX file reads.

## Output Style

Keep terminal output compact:

- one status line per buffer suite;
- detailed compile or runtime output only when something fails;
- deterministic commands that can be copied into CI or a review note.

## Documentation Checklist

When behavior changes, update the relevant files:

- `README.md` for quick-start and feature summaries;
- `docs/USAGE.md` for command workflows;
- `docs/COVERAGE.md` for tested cases;
- `docs/TROUBLESHOOTING.md` for common failures;
- `CHANGELOG.md` for release-visible changes.

## Pull Request Checklist

Before opening a pull request, run:

```sh
make build
./gnl_tester --root /path/to/Get_Next_Line --strict --no-color
./gnl_tester --root /path/to/Get_Next_Line --bonus --strict --no-color
```

If Valgrind is available:

```sh
./gnl_tester --root /path/to/Get_Next_Line --strict --leaks --no-color
```
