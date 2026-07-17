# Release Notes v0.1.0

Initial public version of Get Next Line Tester.

## Highlights

- Standalone `./gnl_tester` binary built with C++17.
- `--root` workflow for testing any local `get_next_line` project.
- Mandatory and bonus modes.
- Multiple `BUFFER_SIZE` profiles, including `--quick` and `--strict`.
- Optional Valgrind leak checks through `--leaks`.
- Documentation for usage, coverage, troubleshooting, and contributions.

## Recommended Command

```sh
make build
./gnl_tester --root /path/to/Get_Next_Line --strict
./gnl_tester --root /path/to/Get_Next_Line --bonus --strict
```
