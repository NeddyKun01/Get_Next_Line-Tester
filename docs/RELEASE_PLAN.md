# Release Plan

This document tracks what must be true before publishing the next version.

## Next Version

Target: `v0.2.0`

Scope:

- reorganized tester source layout;
- shared mandatory and bonus harness utilities;
- timeout-protected test execution;
- review mode;
- optional large-line stress fixtures;
- Valgrind failure classification;
- mandatory pipe fd coverage;
- bonus round-robin coverage across eight file descriptors.

## Checklist

- `README.md` describes the current commands and release status.
- `CHANGELOG.md` has all unreleased changes listed.
- `docs/COVERAGE.md` matches the implemented test cases.
- `docs/TROUBLESHOOTING.md` documents common failures for new checks.
- `docs/RELEASE_NOTES_v0.2.0.md` is updated before tagging.
- `make re` succeeds.
- Mandatory strict mode passes against a known-good target.
- Bonus strict mode passes against a known-good target.
- Review mode passes against a known-good target.

## Validation Commands

```sh
make re
./gnl_tester --root /path/to/Get_Next_Line --strict --no-color
./gnl_tester --root /path/to/Get_Next_Line --bonus --strict --no-color
./gnl_tester --root /path/to/Get_Next_Line --review --no-color
```

## Tagging Steps

1. Move completed items from `Unreleased` to the new version in
   `CHANGELOG.md`.
2. Finalize `docs/RELEASE_NOTES_v0.2.0.md`.
3. Commit the release documentation update.
4. Tag the commit as `v0.2.0`.
5. Push the branch and tag.
