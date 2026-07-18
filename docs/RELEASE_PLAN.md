# Release Plan

This document tracks what must be true before publishing the next version.

## Current Release

Current: `v1.0.0`

Scope:

- doctor mode for environment and target diagnostics;
- header include-guard, `get_next_line` prototype, and Makefile shape checks in
  doctor mode;
- `--diagnose`, `--health`, header compile probes, and lightweight norm-style
  warnings for clearer target checks;
- `--cases`, `--explain CASE`, `--only CASE`, and `--skip CASE` for focused
  debugging with validated case names;
- `--doctor --json` for structured diagnostics;
- `--doctor --web` for standalone diagnostic HTML reports;
- `--export-fixture CASE` and `--rerun-failed FILE` for tighter debug loops;
- `--compact`, `--profile`, `--fd-limit N`, `--norm`, and `--malloc-fail`;
- diagnostics, case filters, failed case labels, and likely-fix hints in JSON
  reports;
- likely-fix hints in Web report failure rows;
- diagnostics in Web reports;
- `--preset pedantic` for a heavier local matrix while `school` remains strict;
- named presets for quick, review, CI, Web, stress, and leak workflows;
- direct JSON and Web output files with `--output`;
- per-process build directories for parallel runs;
- automatic per-run build cleanup with `--keep-build` for debugging;
- `--list`, `--coverage`, `--coverage-md`, and `--compare PATH`;
- optional `.gnl-tester.json` defaults;
- expanded mandatory coverage for stdin-backed reads, closed fds, and generated
  buffer edges, newline-only files, double-buffer boundaries, and read-error
  repeats;
- expanded bonus coverage for wider fd round-robin checks;
- internal self-test fixtures for known-good and known-broken targets;
- GitHub Actions coverage for doctor mode, presets, and report output files;
- GitHub Actions documentation with reusable target-repository examples.

## Maintenance

Target: next patch or minor version

Scope:

- keep `CHANGELOG.md` updated for future changes;
- add release notes for the next version before tagging;
- avoid adding release-generated report files to the repository.

## Checklist

- `README.md` describes the current commands and release status.
- `CHANGELOG.md` has all release changes listed.
- `docs/COVERAGE.md` matches the implemented test cases.
- `docs/TROUBLESHOOTING.md` documents common failures for new checks.
- release notes for the target version are updated before tagging.
- `make re` succeeds.
- `./gnl_tester --doctor` succeeds against a known-good target.
- `./gnl_tester --diagnose` succeeds against a known-good target.
- `./gnl_tester --health` succeeds against a known-good target.
- `./gnl_tester --doctor --json` emits valid JSON diagnostics.
- `./gnl_tester --doctor --web` emits a valid HTML diagnostics report.
- `./gnl_tester --cases` lists every focused case.
- `./gnl_tester --explain stdin` prints a case explanation.
- `--only` and `--skip` focused runs succeed against a known-good target.
- invalid focused case names fail with a useful message.
- fixture export writes expected files.
- rerun-failed reruns failed buffers from a JSON report.
- compact, profile, fd-limit, norm, and malloc-fail modes are smoke-tested.
- `./gnl_tester --presets` lists every documented preset.
- `./gnl_tester --list` and `./gnl_tester --coverage` succeed.
- `make self-test` succeeds.
- Mandatory strict mode passes against a known-good target.
- Bonus strict mode passes against a known-good target.
- Review mode passes against a known-good target.
- JSON output can be written with `--output` and parsed as JSON.
- Web output can be written with `--output` and starts with `<!doctype html>`.
- Parallel JSON and Web runs can complete without build directory collisions.
- Compare mode reports a better score for the known-good fixture against the
  known-broken fixture.

## Validation Commands

```sh
make re
make self-test
./gnl_tester --root /path/to/Get_Next_Line --doctor
./gnl_tester --root /path/to/Get_Next_Line --diagnose
./gnl_tester --root /path/to/Get_Next_Line --health
./gnl_tester --root /path/to/Get_Next_Line --doctor --json
./gnl_tester --root /path/to/Get_Next_Line --doctor --web --output /tmp/gnl-doctor.html
./gnl_tester --cases
./gnl_tester --explain stdin
./gnl_tester --presets
./gnl_tester --list
./gnl_tester --coverage
./gnl_tester --root /path/to/Get_Next_Line --strict --no-color
./gnl_tester --root /path/to/Get_Next_Line --only stdin --quick --no-color
./gnl_tester --root /path/to/Get_Next_Line --skip buffer-edge --quick --no-color
./gnl_tester --root /path/to/Get_Next_Line --preset pedantic --summary-only --no-color
./gnl_tester --root /path/to/Get_Next_Line --only malloc-fail --quick --no-color
./gnl_tester --export-fixture double-buffer --buffer 42 --output /tmp/gnl-fixtures
./gnl_tester --root /path/to/Get_Next_Line --quick --profile --no-color
./gnl_tester --root /path/to/Get_Next_Line --review --compact
./gnl_tester --root /path/to/Get_Next_Line --bonus --strict --no-color
./gnl_tester --root /path/to/Get_Next_Line --review --no-color
./gnl_tester --root /path/to/Get_Next_Line --compare /path/to/Get_Next_Line-before --quick
./gnl_tester --root /path/to/Get_Next_Line --preset ci --output /tmp/gnl-test-report.json
./gnl_tester --root /path/to/Get_Next_Line --preset web --output /tmp/gnl-test-report.html
```

## Tagging Steps

1. Move completed items to the target version in `CHANGELOG.md`.
2. Finalize the matching file under `docs/RELEASE_NOTES_*.md`.
3. Commit the release documentation update.
4. Tag the commit with the target version.
5. Push the branch and tag.
