# GitHub Actions Guide

This guide explains how to run Get Next Line Tester from GitHub Actions.

## Built-In Workflow

This repository includes `.github/workflows/ci.yml`.

The workflow always:

- builds `gnl_tester`;
- runs internal smoke checks;
- runs `make self-test`;
- validates JSON and Web report output;
- uploads report artifacts when report jobs run.

To also test an external Get Next Line repository, set this repository variable:

```text
GNL_REPOSITORY=owner/repository
```

You can also start the workflow manually and provide `gnl_repository` through
`workflow_dispatch`.

## Uploaded Artifacts

Configured target-project runs can upload:

| Artifact | Purpose |
| --- | --- |
| `gnl-review.txt` | Compact terminal review summary. |
| `gnl-doctor.json` | Structured setup and target diagnostics. |
| `gnl-doctor.html` | Standalone diagnostic HTML report. |
| `gnl-test-report.json` | Machine-readable review result. |
| `gnl-test-report.html` | Standalone Web dashboard with likely-fix hints. |

Release-generated reports should stay out of git. Download them from the
workflow run when needed.

## Target Repository Example

Add this workflow to a Get Next Line repository when you want it to run the
tester directly:

```yaml
name: Get Next Line Tester

on:
  push:
  pull_request:
  workflow_dispatch:

jobs:
  gnl-tester:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout target
        uses: actions/checkout@v4
        with:
          path: target

      - name: Checkout tester
        uses: actions/checkout@v4
        with:
          repository: NeddyKun01/Get_Next_Line-Tester
          path: tester

      - name: Build tester
        working-directory: tester
        run: make build

      - name: Diagnose target
        working-directory: tester
        run: |
          ./gnl_tester --root ../target --doctor --json --output gnl-doctor.json
          ./gnl_tester --root ../target --doctor --web --output gnl-doctor.html

      - name: Run review
        working-directory: tester
        run: ./gnl_tester --root ../target --preset review --no-color

      - name: Write JSON report
        working-directory: tester
        run: ./gnl_tester --root ../target --preset ci --output gnl-test-report.json

      - name: Write Web report
        working-directory: tester
        run: ./gnl_tester --root ../target --preset web --output gnl-test-report.html

      - name: Upload reports
        uses: actions/upload-artifact@v4
        with:
          name: gnl-tester-reports
          path: |
            tester/gnl-test-report.json
            tester/gnl-test-report.html
            tester/gnl-doctor.json
            tester/gnl-doctor.html
```

## Focused Debug Example

Use a smaller job while debugging one failure:

```yaml
- name: Focus stdin case
  working-directory: tester
  run: ./gnl_tester --root ../target --only stdin --quick --no-color
```

Use `--health` for a lightweight status check:

```yaml
- name: Health summary
  working-directory: tester
  run: ./gnl_tester --root ../target --health
```
