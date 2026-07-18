# Get Next Line Tester v0.4.0

This release focuses on cleaner output and faster failure feedback.

## Highlights

- Added `--summary-only` for cleaner automation and CI-style logs.
- Added `--fail-fast` to stop after the first failing buffer suite.
- Kept failing diagnostics visible in normal `--summary-only` runs.
- Made `--summary-only` keep review mode focused on the compact summary.
- Documented the new output controls in the README, usage guide, coverage notes,
  and changelog.

## Recommended Validation

```sh
make re
./gnl_tester --root /path/to/Get_Next_Line --strict --no-color
./gnl_tester --root /path/to/Get_Next_Line --bonus --strict --no-color
./gnl_tester --root /path/to/Get_Next_Line --review --no-color
```

## Validation

This release was validated locally with summary-only mode, fail-fast mode,
review mode, and a controlled failing target that confirms fail-fast stops after
the first failing buffer suite.
