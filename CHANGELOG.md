# Changelog

All notable changes to this project will be documented in this file.

## Unreleased

- Reorganized the tester source layout with shared declarations in
  `tester/include/` and harness definitions in `tester/tests/`.
- Added shared C harness utilities so mandatory and bonus tests do not duplicate
  fixture and assertion helpers.
- Planned: timeout-protected test execution.
- Planned: larger stress fixtures for very long lines.
- Planned: reviewer-oriented summary mode.
- Planned: clearer Valgrind leak classification.

## 0.1.0 - 2026-07-15

- Added the initial standalone `gnl_tester` C++17 driver.
- Added `--root` support for testing any local `get_next_line` project.
- Added mandatory checks for invalid fds, empty files, EOF behavior, files with
  and without final newlines, blank lines, long lines, and buffer boundaries.
- Added `--bonus` mode for interleaved multi-fd checks using the bonus sources.
- Added configurable `BUFFER_SIZE` runs through `--buffer`, `--quick`, and
  `--strict`.
- Added optional `--leaks` support through Valgrind when available.
- Added a simple `Makefile` with `build`, `clean`, `fclean`, and `re`.
- Added project documentation and contribution notes.
