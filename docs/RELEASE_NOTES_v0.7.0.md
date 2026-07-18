# Release Notes v0.7.0

Get Next Line Tester `v0.7.0` adds standalone Web dashboard reports for local
runs and GitHub Actions artifacts.

## Highlights

- Added `--web` and `--html` for self-contained HTML reports.
- Added Web dashboard generation to the GitHub Actions target-project workflow.
- Added a `gnl-web-report` artifact containing `gnl-test-report.html`.
- Documented Web dashboard usage, coverage, and release status.

## Web Dashboard

Generate a local report with:

```sh
./gnl_tester --root ../Get_Next_Line --review --web > gnl-test-report.html
```

The dashboard includes:

- run configuration;
- mandatory and bonus suite summaries;
- per-buffer result rows;
- compile, test, timeout, and Valgrind failure output;
- row filters for all, passed, and failed results;
- copyable rerun commands for failing buffer suites.

## Validation

Before publishing this release, validate with:

```sh
make re
./gnl_tester --help
./gnl_tester --root /path/to/Get_Next_Line --strict --no-color
./gnl_tester --root /path/to/Get_Next_Line --bonus --strict --no-color
./gnl_tester --root /path/to/Get_Next_Line --review --no-color
./gnl_tester --root /path/to/Get_Next_Line --review --json
./gnl_tester --root /path/to/Get_Next_Line --review --web > gnl-test-report.html
```
