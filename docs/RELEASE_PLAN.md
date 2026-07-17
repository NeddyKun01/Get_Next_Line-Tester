# Release Plan

This document tracks what must be true before publishing the next version.

## Current Release

Current: `v0.2.0`

Scope:

- reorganized tester source layout;
- shared mandatory and bonus harness utilities;
- timeout-protected test execution;
- review mode;
- optional large-line stress fixtures;
- Valgrind failure classification;
- mandatory pipe fd coverage;
- bonus round-robin coverage across eight file descriptors.

## Next Version

Target: `v0.3.0`

Scope:

- keep `CHANGELOG.md` updated under `Unreleased`;
- expand coverage only when the expected behavior is clear;
- keep release notes aligned with the final changelog before tagging.

## Checklist

- `README.md` describes the current commands and release status.
- `CHANGELOG.md` has all unreleased changes listed.
- `docs/COVERAGE.md` matches the implemented test cases.
- `docs/TROUBLESHOOTING.md` documents common failures for new checks.
- release notes for the target version are updated before tagging.
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

1. Move completed items from `Unreleased` to the target version in
   `CHANGELOG.md`.
2. Finalize the matching file under `docs/RELEASE_NOTES_*.md`.
3. Commit the release documentation update.
4. Tag the commit with the target version.
5. Push the branch and tag.
