# Get Next Line Tester v0.6.0

This release adds machine-readable JSON output for CI and scripts.

## Highlights

- Added `--json` for normal mandatory or bonus runs.
- Added `--json` support for review mode.
- JSON mode disables color automatically and writes only JSON to stdout.
- JSON output includes selected buffers, pass/fail summary, per-buffer status,
  timeout state, and Valgrind issue categories.
- GitHub Actions now generates and uploads `gnl-test-report.json`.
- README, usage guide, coverage notes, changelog, and release plan now document
  JSON output.

## Recommended Validation

```sh
make re
./gnl_tester --root /path/to/Get_Next_Line --quick --json
./gnl_tester --root /path/to/Get_Next_Line --review --json
./gnl_tester --root /path/to/Get_Next_Line --strict --no-color
./gnl_tester --root /path/to/Get_Next_Line --bonus --strict --no-color
```

## Validation

This release was validated locally with JSON parsing, quick JSON mode, review
JSON mode, mandatory strict mode, bonus strict mode, and the existing review
workflow against a known-good Get Next Line target.
