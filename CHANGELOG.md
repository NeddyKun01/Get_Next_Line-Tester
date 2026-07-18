# Changelog

All notable changes to this project will be documented in this file.

## Unreleased

## 0.7.0 - 2026-07-18

- Added `--web` and `--html` for standalone Web dashboard reports.
- Added Web report generation and artifact upload to GitHub Actions.
- Added documentation for Web dashboard usage, coverage, and release status.

## 0.6.0 - 2026-07-18

- Added `--json` for machine-readable normal and review output.
- Added JSON report generation and artifact upload to GitHub Actions.

## 0.5.0 - 2026-07-18

- Added GitHub Actions for tester smoke checks and optional target-project CI.
- Added custom issue templates for bug reports, feature requests, and questions.

## 0.4.0 - 2026-07-18

- Added `--summary-only` for cleaner automation output.
- Added `--fail-fast` to stop after the first failing buffer suite.

## 0.3.0 - 2026-07-18

- Reworked the README structure to match the full public documentation layout.

## 0.2.0 - 2026-07-17

- Reorganized the tester source layout with shared declarations in
  `tester/include/` and harness definitions in `tester/tests/`.
- Added shared C harness utilities so mandatory and bonus tests do not duplicate
  fixture and assertion helpers.
- Added `--timeout MS` to kill stuck test runs.
- Added `--review` for compact mandatory and bonus pre-submission summaries.
- Added `--stress` for opt-in 10k and 100k mandatory line fixtures.
- Added Valgrind failure classification for leaks and common memory errors.
- Added mandatory pipe fd coverage.
- Added bonus round-robin coverage across eight file descriptors.
- Added a release plan document for the next published version.
- Added release notes for `v0.2.0`.

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
