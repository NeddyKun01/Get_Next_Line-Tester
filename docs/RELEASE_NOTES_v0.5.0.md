# Get Next Line Tester v0.5.0

This release adds GitHub project automation and issue intake templates.

## Highlights

- Added a GitHub Actions workflow for tester smoke checks.
- Added optional target-project CI through `GNL_REPOSITORY` or manual
  `workflow_dispatch` input.
- Added mandatory strict, bonus strict, review-summary, and Valgrind jobs for
  configured target repositories.
- Added upload of the review summary artifact.
- Added custom issue templates for bug reports, feature requests, and questions.
- Documented CI setup in the README and usage guide.

## Recommended Validation

```sh
make re
./gnl_tester --root /path/to/Get_Next_Line --strict --summary-only --fail-fast --no-color
./gnl_tester --root /path/to/Get_Next_Line --bonus --strict --summary-only --fail-fast --no-color
./gnl_tester --root /path/to/Get_Next_Line --review --summary-only --no-color
```

## Validation

This release was validated locally with YAML parsing, tester rebuild, mandatory
strict mode, bonus strict mode, and review mode against a known-good Get Next
Line target.
