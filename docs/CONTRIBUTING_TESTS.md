# Contributing Tests

This guide explains how to add or change test coverage.

## Current Structure

The tester uses a compact generated-harness model:

```text
tester/src/main.cpp
```

At runtime, the C++ driver writes a temporary `harness.c`, compiles it with the
target `get_next_line` files, and runs the resulting executable for each
selected `BUFFER_SIZE`.

## Adding Mandatory Cases

Mandatory cases live in the non-bonus branch of `harness_source()` in
`tester/src/main.cpp`.

Good new cases should include:

- the fixture content;
- the exact expected line sequence;
- one final `NULL` expectation;
- a repeated EOF call when it protects against stale state.

## Adding Bonus Cases

Bonus cases live in the bonus branch of `harness_source()`.

Prefer interleaved reads that prove independent fd state:

```text
fd A -> first line
fd B -> first line
fd A -> second line
fd B -> second line
```

Add EOF checks for each fd separately.

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
