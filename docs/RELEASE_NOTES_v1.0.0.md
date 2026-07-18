# Release Notes v1.0.0

Get Next Line Tester `v1.0.0` is the first broad stability and workflow
release. It focuses on diagnostics, repeatable presets, cleaner report
generation, safer parallel execution, and stronger mandatory coverage.

## Highlights

- Added `--doctor` for tool, tester-file, mandatory-file, and bonus-file checks.
- Expanded doctor mode with header include-guard checks, `get_next_line`
  prototype checks, and Makefile shape checks when present.
- Added `--diagnose` and `--health`.
- Added header compile probes and lightweight norm-style warnings.
- Added `--cases`, `--explain CASE`, `--only CASE`, and `--skip CASE`.
- Added validation for focused case names.
- Added `--doctor --json` for structured diagnostics.
- Added `--doctor --web` for standalone diagnostic HTML reports.
- Added `--export-fixture CASE` and `--rerun-failed FILE`.
- Added `--compact`, `--profile`, `--fd-limit N`, `--norm`, and
  `--malloc-fail`.
- Added diagnostics, case filters, failed case labels, and likely-fix hints to
  JSON reports.
- Added diagnostics to Web reports.
- Added `--preset pedantic` while keeping `school` as a strict preset.
- Added named presets with `--preset NAME`.
- Added `--presets` to list every preset and its expansion.
- Added `--output FILE` for JSON and Web reports.
- Added `--version` and report version metadata.
- Added `--list`, `--coverage`, and `--coverage-md`.
- Added `--compare PATH` for comparing two target roots.
- Added optional `.gnl-tester.json` defaults.
- Added `--keep-build` and automatic cleanup of per-run build directories.
- Added per-process build directories to support parallel tester runs.
- Added mandatory `STDIN_FILENO` coverage.
- Added mandatory closed-fd and generated buffer-edge coverage.
- Added newline-only, many-newline, double-buffer boundary, and read-error
  repeat coverage.
- Added wider bonus fd round-robin coverage.
- Added internal known-good and known-broken fixtures.
- Added `make self-test` and `make clean-runs`.
- Added Makefile targets for doctor, presets, review, JSON reports, and Web
  reports.
- Added likely-fix hints to failing rows in Web reports.
- Added a GitHub Actions guide with reusable target-repository examples.
- Improved empty failure output messages.
- Updated GitHub Actions to validate doctor mode, presets, and report output
  files.

## Presets

Available presets:

| Preset | Purpose |
| --- | --- |
| `quick` | Fast local feedback while editing. |
| `normal` | Balanced default run. |
| `strict` | Wider buffer matrix for stronger checks. |
| `school` | Alias for strict pre-submission checks. |
| `pedantic` | Heavier local matrix with stress enabled. |
| `review` | Compact mandatory and bonus review flow. |
| `ci` | JSON review report with CI-friendly defaults. |
| `web` | Standalone Web review dashboard. |
| `html` | Alias for the Web dashboard preset. |
| `stress` | Large-line mandatory fixtures with practical buffer sizes. |
| `leaks` | Focused Valgrind leak checks. |

## New Commands

```sh
./gnl_tester --version
./gnl_tester --presets
./gnl_tester --root ../Get_Next_Line --doctor
./gnl_tester --root ../Get_Next_Line --diagnose
./gnl_tester --root ../Get_Next_Line --health
./gnl_tester --root ../Get_Next_Line --doctor --json
./gnl_tester --root ../Get_Next_Line --doctor --web --output gnl-doctor.html
./gnl_tester --cases
./gnl_tester --explain stdin
./gnl_tester --root ../Get_Next_Line --only stdin --quick
./gnl_tester --root ../Get_Next_Line --skip buffer-edge --strict
./gnl_tester --root ../Get_Next_Line --preset pedantic
./gnl_tester --export-fixture double-buffer --buffer 42 --output exported-fixtures
./gnl_tester --root ../Get_Next_Line --rerun-failed gnl-test-report.json
./gnl_tester --root ../Get_Next_Line --review --compact
./gnl_tester --list
./gnl_tester --coverage
./gnl_tester --root ../Get_Next_Line --preset review
./gnl_tester --root ../Get_Next_Line --compare ../Get_Next_Line-before --quick
./gnl_tester --root ../Get_Next_Line --preset ci --output gnl-test-report.json
./gnl_tester --root ../Get_Next_Line --preset web --output gnl-test-report.html
```

## Validation

Before publishing this release, validate with:

```sh
make re
make self-test
./gnl_tester --root /path/to/Get_Next_Line --doctor
./gnl_tester --root /path/to/Get_Next_Line --diagnose
./gnl_tester --root /path/to/Get_Next_Line --health
./gnl_tester --root /path/to/Get_Next_Line --doctor --json
./gnl_tester --root /path/to/Get_Next_Line --doctor --web --output /tmp/gnl-doctor.html
./gnl_tester --cases
./gnl_tester --explain stdin
./gnl_tester --presets
./gnl_tester --list
./gnl_tester --coverage
./gnl_tester --root /path/to/Get_Next_Line --strict --no-color
./gnl_tester --root /path/to/Get_Next_Line --only stdin --quick --no-color
./gnl_tester --root /path/to/Get_Next_Line --skip buffer-edge --quick --no-color
./gnl_tester --root /path/to/Get_Next_Line --preset pedantic --summary-only --no-color
./gnl_tester --root /path/to/Get_Next_Line --only malloc-fail --quick --no-color
./gnl_tester --export-fixture double-buffer --buffer 42 --output /tmp/gnl-fixtures
./gnl_tester --root /path/to/Get_Next_Line --quick --profile --no-color
./gnl_tester --root /path/to/Get_Next_Line --review --compact
./gnl_tester --root /path/to/Get_Next_Line --bonus --strict --no-color
./gnl_tester --root /path/to/Get_Next_Line --review --no-color
./gnl_tester --root /path/to/Get_Next_Line --compare /path/to/Get_Next_Line-before --quick
./gnl_tester --root /path/to/Get_Next_Line --preset ci --output /tmp/gnl-test-report.json
./gnl_tester --root /path/to/Get_Next_Line --preset web --output /tmp/gnl-test-report.html
```
