# Get Next Line Tester v0.2.0

This release focuses on repository organization, reviewer-friendly workflows,
and stronger mandatory and bonus coverage.

## Highlights

- Reorganized tester source layout with shared headers and separate harnesses.
- Shared fixture and assertion helpers between mandatory and bonus tests.
- Review mode for compact pre-submission summaries.
- Timeout-protected test execution.
- Mandatory pipe fd coverage.
- Bonus round-robin coverage across eight file descriptors.
- Opt-in large-line stress fixtures with `--stress`.
- Valgrind failure classification for common memory errors and leaks.

## Recommended Validation

```sh
make re
./gnl_tester --root /path/to/Get_Next_Line --strict --no-color
./gnl_tester --root /path/to/Get_Next_Line --bonus --strict --no-color
./gnl_tester --root /path/to/Get_Next_Line --review --no-color
```

## Validation

This release was validated locally with mandatory strict mode, bonus strict
mode, and review mode against a known-good Get Next Line target.
