---
name: Bug report
about: Report a problem in Get Next Line Tester
title: "[Bug]: "
labels: bug
assignees: ""
---

## Summary

Describe the problem clearly.

## Area Affected

Choose the closest area:

- CLI
- `--review`
- `--summary-only`
- `--fail-fast`
- mandatory tests
- bonus tests
- stress tests
- leak checks
- CI
- docs
- release
- other

## Steps To Reproduce

1. Set up the tester and target Get Next Line project.
2. Run the command that fails.
3. Paste the relevant output below.

Example commands:

```sh
make build
./gnl_tester --root ../Get_Next_Line --strict --no-color
./gnl_tester --root ../Get_Next_Line --bonus --strict --no-color
./gnl_tester --root ../Get_Next_Line --review --summary-only --no-color
./gnl_tester --root ../Get_Next_Line --strict --leaks --no-color
```

## Expected Behavior

What did you expect to happen?

## Actual Behavior

What happened instead?

## Environment

- OS:
- Shell:
- Compiler and version:
- Make version:
- Valgrind version, if relevant:
- Tester version or commit:
- Target repository or commit:
- Command and flags used:
- `BUFFER_SIZE` values used:

## Output

Paste relevant terminal output, CI logs, Valgrind output, or report snippets.

```text
paste output here
```

## Additional Context

Mention affected buffer sizes, mandatory or bonus mode, leak-check behavior, or
anything else that may help reproduce the issue.
