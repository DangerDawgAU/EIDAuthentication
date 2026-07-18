# Aikido Security CI Gate — Design

**Date:** 2026-07-18
**Branch:** `main`
**Scope:** Add one GitHub Actions workflow that runs the Aikido PR gate.

## Goal

Get the maximum security value available on Aikido's **free tier** through GitHub Actions:
hard-fail pull requests on new **dependency (SCA) CVEs**, and surface all other findings
(SAST, IaC, secrets) on the PR as non-blocking comments.

## Context

Aikido integration has two parts:

1. **GitHub App connection** (configured on aikido.dev — *not* a workflow). Already done by
   the maintainer. This runs the scanners (SAST, SCA, secrets, IaC) on Aikido's cloud and
   rescans on the free-tier cadence (~every 3 days). This is where the bulk of scanning
   happens.
2. **This workflow** — a CI gate using `AikidoSec/github-actions-workflow`. It triggers a
   scan of the PR change, can fail the build, and posts findings to the PR.

**Free-tier constraint:** the Action can only *enforce* (fail CI) on dependency findings.
SAST/IaC/license blocking requires a paid plan. Those scanners still run via the App and
their findings are surfaced via the PR comment; they just don't fail the build.

## Decisions

| Decision | Value | Rationale |
|----------|-------|-----------|
| Trigger | `pull_request` (all target branches) | Aikido's recommended event; lets the Action comment on the PR. No `push` trigger — the App already rescans `main`, so a push gate would be redundant and can't comment. |
| Runner | `ubuntu-latest` | The Action only calls Aikido's API; no build required, so it's fast and consumes no Windows-runner minutes. |
| Fail threshold | `minimum-severity: HIGH` | Fails on HIGH **and** CRITICAL dependency CVEs (HIGH is the floor). Appropriate for a security-sensitive project. |
| Dependency gate | `fail-on-dependency-scan: true` | The one gate the free tier enforces. |
| SAST / IaC gate | `false` | Scanned + surfaced but non-blocking on free tier. Flip to `true` if upgraded. |
| PR status comment | `post-scan-status-comment: only_if_new_findings` | Surface findings without noise on clean PRs. |
| SAST review comments | `post-sast-review-comments: on` | Inline SAST feedback on the PR (informational). |
| Secret name | `AIKIDO_KEY` | Repo secret holding the CI integration API key. |
| Action version | `AikidoSec/github-actions-workflow@v1.0.13` | Current pinned release at time of writing. |

## Workflow shape

File: `.github/workflows/aikido.yml`

```yaml
name: Aikido Security

on:
  pull_request:

permissions:
  contents: read
  pull-requests: write   # required to post PR comments

jobs:
  aikido:
    runs-on: ubuntu-latest
    steps:
      - uses: AikidoSec/github-actions-workflow@v1.0.13
        with:
          secret-key: ${{ secrets.AIKIDO_KEY }}
          minimum-severity: HIGH
          fail-on-dependency-scan: true
          fail-on-sast-scan: false
          fail-on-iac-scan: false
          post-scan-status-comment: only_if_new_findings
          post-sast-review-comments: 'on'
          github-token: ${{ secrets.GITHUB_TOKEN }}
          timeout-seconds: 300
```

## Manual prerequisites (one-time, outside this repo)

1. Aikido → **Continuous Integration settings** → generate a CI integration token
   (viewable once).
2. GitHub → repo **Settings → Secrets and variables → Actions** → add secret `AIKIDO_KEY`
   with that token.

## Behavior summary

- New dependency CVE ≥ HIGH on a PR → **CI fails**.
- SAST / IaC / secrets findings → posted to the PR (status comment + inline SAST comments),
  **non-blocking**.
- Clean PR → passes silently (no comment).

## Out of scope

- Aikido "local scanner" (on-prem scanning) — more involved, not needed for free tier.
- Enforcing SAST/IaC blocking — requires a paid plan; the workflow is pre-wired to enable it
  with a one-line flip if that changes.
- Any change to existing workflows (`windows-ci.yaml`, `codeql.yml`, `scan-artifacts.yml`,
  `release-vt-scan.yml`).
</content>
</invoke>
