# Usage Guide

Get Next Line Tester has two workflows:

- use `./gnl_tester` for direct commands, scripts, and CI;
- use `make` as a convenience wrapper when you want it.

## Project Layouts

The tester can live next to your target repository:

```text
projects/
├── Get_Next_Line/
└── Get_Next_Line-Tester/
```

```sh
cd Get_Next_Line-Tester
make build
./gnl_tester --root ../Get_Next_Line
```

The tester can also live anywhere else when you pass an absolute path:

```sh
./gnl_tester --root /absolute/path/to/Get_Next_Line
```

## Minimal Make Commands

| Command | Purpose |
| --- | --- |
| `make build` | Build the standalone `./gnl_tester` driver. |
| `make ROOT_DIR=../Get_Next_Line` | Build and run the default suite. |
| `make clean` | Remove tester build files and reports. |
| `make fclean` | Same as `clean`. |
| `make re` | Rebuild the driver. |

## Direct CLI Workflow

Build the driver first:

```sh
make build
```

Then run the binary:

```sh
./gnl_tester --root ../Get_Next_Line
./gnl_tester --root ../Get_Next_Line --quick
./gnl_tester --root ../Get_Next_Line --strict
./gnl_tester --root ../Get_Next_Line --bonus --strict
./gnl_tester --root ../Get_Next_Line --buffer 1,2,42,1024
./gnl_tester --root ../Get_Next_Line --strict --timeout 5000
./gnl_tester --root ../Get_Next_Line --strict --leaks --no-color
./gnl_tester --help
```

## Buffer Profiles

| Option | Buffer sizes | Best for |
| --- | --- | --- |
| default | `1,2,3,5,8,16,32,42,128,1024` | Normal local validation. |
| `--quick` | `1,42` | Fast feedback while editing. |
| `--strict` | `1,2,3,4,5,7,8,16,32,42,64,128,1024` | Stronger pre-review checks. |
| `--buffer LIST` | custom comma-separated list | Focused debugging. |

The tester recompiles the target once per `BUFFER_SIZE`:

```text
cc -Wall -Wextra -Werror -D BUFFER_SIZE=N
```

This catches bugs that only appear with tiny buffers, larger buffers, or exact
boundary lengths.

## Timeout

Each compiled test run is killed if it exceeds the configured timeout. The
default is `3000` milliseconds.

Use a larger timeout for slow machines or heavy Valgrind runs:

```sh
./gnl_tester --root ../Get_Next_Line --strict --timeout 5000
```

Use `--timeout 0` to disable the timeout.

## Bonus Mode

Use `--bonus` when the target repository includes:

```text
get_next_line_bonus.c
get_next_line_utils_bonus.c
get_next_line_bonus.h
```

Example:

```sh
./gnl_tester --root ../Get_Next_Line --bonus --strict
```

Bonus mode checks that two file descriptors can be read in an interleaved order
without their internal state leaking into each other.

## Leak Checks

Use `--leaks` to run each compiled suite under Valgrind when it is installed:

```sh
./gnl_tester --root ../Get_Next_Line --strict --leaks --no-color
```

If Valgrind is missing, the tester skips leak checks and prints a note.
