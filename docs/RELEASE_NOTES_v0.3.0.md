# Get Next Line Tester v0.3.0

This release focuses on README quality and public documentation structure.

## Highlights

- Reworked the README into a fuller public project guide.
- Added top-level badges for language, project, mode, release, and license.
- Added clear sections for requirements, quick start, first-run workflow,
  result statuses, buffer profiles, advanced CLI usage, Makefile commands, leak
  checks, release status, documentation, contributing, and license.
- Kept the README aligned with the tester's actual command surface.
- Updated release planning so future versions have a clear checklist.

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
