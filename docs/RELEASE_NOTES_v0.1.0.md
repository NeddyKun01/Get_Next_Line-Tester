# Release Notes v0.1.0

Initial public version of Get Next Line Tester.

## Highlights

- Standalone `./gnl_tester` binary built with C++17.
- `--root` workflow for testing any local `get_next_line` project.
- Mandatory and bonus modes.
- Multiple `BUFFER_SIZE` profiles, including `--quick` and `--strict`.
- Review mode for compact pre-submission summaries.
- Timeout-protected test execution.
- Pipe fd coverage in mandatory mode.
- Round-robin bonus coverage across eight file descriptors.
- Opt-in large-line stress fixtures with `--stress`.
- Optional Valgrind leak checks through `--leaks`.
- Valgrind failure classification for common memory errors and leaks.
- Documentation for usage, coverage, troubleshooting, and contributions.

## Recommended Command

```sh
make build
./gnl_tester --root /path/to/Get_Next_Line --strict
./gnl_tester --root /path/to/Get_Next_Line --bonus --strict
./gnl_tester --root /path/to/Get_Next_Line --review
```
