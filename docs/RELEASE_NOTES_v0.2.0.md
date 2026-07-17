# Release Notes v0.2.0

Draft release notes for the next version of Get Next Line Tester.

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

## Release Status

This release is not tagged yet. Keep this file aligned with `CHANGELOG.md`
until `v0.2.0` is published.
