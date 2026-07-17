# Contributing Tests

This guide explains how to add or change test coverage.

## Current Structure

The tester uses a compact generated-harness model:

```text
tester/include/gnl_tester.hpp
tester/src/main.cpp
tester/tests/harness_utils.h
tester/tests/harness_utils.c
tester/tests/mandatory_harness.c
tester/tests/bonus_harness.c
```

At runtime, the C++ driver selects the mandatory or bonus C harness from
`tester/tests/`, compiles it with the shared harness utilities and the target
`get_next_line` files, and runs the resulting executable for each selected
`BUFFER_SIZE`.

## Adding Mandatory Cases

Mandatory cases live in:

```text
tester/tests/mandatory_harness.c
```

Good new cases should include:

- the fixture content;
- the exact expected line sequence;
- one final `NULL` expectation;
- a repeated EOF call when it protects against stale state.

## Adding Bonus Cases

Bonus cases live in:

```text
tester/tests/bonus_harness.c
```

Prefer interleaved reads that prove independent fd state:

```text
fd A -> first line
fd B -> first line
fd A -> second line
fd B -> second line
```

Add EOF checks for each fd separately.

## Shared Harness Helpers

Common helper functions live in:

```text
tester/tests/harness_utils.c
tester/tests/harness_utils.h
```

Use those helpers for fixture creation, path joining, line comparison, and
shared failure counting. Keep individual harness files focused on test cases.

## Guidelines

- Keep fixtures short unless the test is specifically about long input.
- Prefer readable expected strings over clever generation.
- Do not require undefined behavior.
- Keep failure labels specific enough to identify the broken fixture.
- Run both mandatory and bonus strict suites after changing shared logic.

## Validation Commands

```sh
make build
./gnl_tester --root /path/to/Get_Next_Line --strict --no-color
./gnl_tester --root /path/to/Get_Next_Line --bonus --strict --no-color
```

When changing leak-sensitive behavior:

```sh
./gnl_tester --root /path/to/Get_Next_Line --strict --leaks --no-color
```
