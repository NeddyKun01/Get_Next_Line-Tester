# Changelog

All notable changes to this project will be documented in this file.

## 1.0.0 - 2026-07-19

- Prepared the `v1.0.0` development cycle.
- Added `--doctor` for local tool and target project diagnostics.
- Expanded doctor mode with header include-guard checks, `get_next_line`
  prototype checks, and Makefile shape checks when a target Makefile exists.
- Added `--diagnose` and `--health` for target-project diagnostics and compact
  health summaries.
- Added header compile probes and lightweight norm-style warnings to
  diagnostics.
- Added `--explain CASE`, `--only CASE`, and `--skip CASE` for focused
  debugging.
- Added `--cases` and validation for focused case names.
- Added `--preset pedantic` for a heavier local matrix while keeping `school`
  as a strict preset.
- Added `--doctor --json` for structured diagnostics.
- Added `--doctor --web` for standalone diagnostic HTML reports.
- Added `--export-fixture CASE` for writing representative case fixtures.
- Added `--rerun-failed FILE` for rerunning failed buffers from a JSON report.
- Added `--compact` and `--profile` terminal output modes.
- Added `--fd-limit N` for wide bonus fd checks.
- Added optional `--norm` diagnostics.
- Added `--malloc-fail` allocation-failure harness checks.
- Added diagnostics, case filters, failed case labels, and likely-fix hints to
  JSON reports.
- Added a diagnostics section to Web reports.
- Added newline-only, many-newline, double-buffer boundary, read-error repeat,
  and wide bonus fd coverage.
- Added likely-fix hints to failing rows in Web reports.
- Added named presets with `--preset NAME` and preset listing with `--presets`.
- Added `--output FILE` for JSON and Web reports.
- Added `--version` with tester version metadata in JSON and Web reports.
- Added `--list`, `--coverage`, and `--coverage-md` for inspecting tester
  capabilities from the CLI.
- Added `--compare PATH` for comparing two target roots with the same selected
  options.
- Added optional `.gnl-tester.json` defaults.
- Added `--keep-build` while making per-run build directories clean up by
  default.
- Added per-process build directories so parallel tester runs do not overwrite
  each other's logs or fixtures.
- Added mandatory `STDIN_FILENO` coverage.
- Added mandatory closed-fd and generated buffer-edge coverage.
- Added internal known-good and known-broken target fixtures.
- Added `make self-test` and `make clean-runs`.
- Added Makefile targets for doctor mode, presets, review, JSON reports, and
  Web reports.
- Improved empty failure output messages for terminal, JSON, and Web reports.
- Updated GitHub Actions to validate doctor mode, presets, and output-file
  report generation.
- Added a GitHub Actions guide with reusable target-repository examples.

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
