# Single-Pane-of-Glass PR Report — Design

**Date:** 2026-07-18
**Branch:** `ci/pr-report` (base `main`)
**Scope:** Add one GitHub Actions workflow that, after all checks for a PR finish,
collates every action's results **and findings** into one report, posted as a sticky
PR comment and a job-summary dashboard.

## Goal

A "single pane of glass" per PR: overall status, PR diff stats, a table of every check,
quality metrics, and — importantly — **the actual findings** from each tool collated in one
place (CodeQL alerts, SonarCloud issues/hotspots, Aikido findings, and any warnings/errors
any action emitted). No email (per decision); GitHub-hosted only.

## Trigger & timing

- **Trigger:** `workflow_run` on the **CI** workflow, `types: [completed]`. CI runs on every
  PR, so this fires once CI finishes. The triggering event carries the PR number and head SHA
  (`github.event.workflow_run.pull_requests[0].number`, `.head_sha`).
- **Wait for all checks:** because SonarCloud and Aikido are GitHub **Apps** (not workflows)
  and CodeQL is a separate workflow, `workflow_run` cannot key on them. The job therefore
  **polls** `GET /repos/{o}/{r}/commits/{head_sha}/check-runs` until every check-run is
  `status == completed` (excluding its own), sleeping ~30 s, with a ~20-min safety timeout.
  This yields one complete snapshot per push.

> **Activation note:** `workflow_run` workflows execute from the **default-branch** copy of
> the file. So this report does **not** run for the PR that introduces it — it starts working
> for PRs opened after it lands on `main`. Verify with a follow-up throwaway PR.

## Architecture

A single workflow `.github/workflows/pr-report.yml` with one job on `ubuntu-latest`, doing
all work in one `actions/github-script@v8` step (first-party — no third-party action to pin).

### Permissions (least privilege, per-job)

```yaml
permissions:
  pull-requests: write     # create/update the sticky comment
  checks: read             # list check-runs + annotations
  security-events: read    # code-scanning (CodeQL) alerts
  contents: read
  actions: read            # workflow-run timing details
```

### Data sources

| Section | Source | Endpoint |
|---------|--------|----------|
| PR metadata / diff stats | GitHub | `GET /repos/{o}/{r}/pulls/{n}` (title, user, commits, changed_files, additions, deletions) |
| Check results table | GitHub | `GET /repos/{o}/{r}/commits/{sha}/check-runs` (name, conclusion, started/completed → duration, html_url, output.summary) |
| Findings — catch-all | GitHub | `GET /repos/{o}/{r}/check-runs/{id}/annotations` for each check (path, line, level, message) — ensures nothing any action reported is missed |
| CodeQL alerts | GitHub | `GET /repos/{o}/{r}/code-scanning/alerts?ref=refs/pull/{n}/merge` (rule, severity, file:line, state) |
| SonarCloud gate + metrics | SonarCloud public API (no auth) | `qualitygates/project_status` + `measures/component` (new_bugs, new_vulnerabilities, new_code_smells, new_coverage, new_duplicated_lines_density) with `pullRequest={n}` |
| SonarCloud findings | SonarCloud public API | `issues/search` + `hotspots/search` with `pullRequest={n}` |
| Aikido status (PR-scoped) | GitHub | the `Aikido Security: check code` check-run: conclusion + output.summary + details_url |
| Aikido findings (itemized) | Aikido REST API | see below |
| VirusTotal | — | not run on PRs (its trigger is push-to-main); shown as "n/a for PRs" |

### Aikido REST API integration

- **Auth:** OAuth 2.0 client credentials. `POST https://app.aikido.dev/api/oauth/token`
  with `grant_type=client_credentials` and an **HTTP Basic** header of
  `base64(client_id:client_secret)`. Response → `access_token`; use as
  `Authorization: Bearer <token>`.
- **Findings:** `GET https://app.aikido.dev/api/public/v1/issues/export?format=json&filter_status=open&filter_code_repo_name=EIDAuthentication`
  (pageable via `page`/`per_page`). Returns itemized open issues (type, severity, status,
  location, description).
- **Important scope caveat:** the REST API filters by **repository, not branch/PR**. So the
  itemized Aikido findings reflect the **repo's current open issues**, not just this PR's diff.
  The PR-scoped pass/fail stays sourced from the Aikido **check-run**. The report labels the
  Aikido findings section "repo-wide open issues" to avoid implying they're PR-diff-scoped.
- **Secrets:** `AIKIDO_API_CLIENT_ID`, `AIKIDO_API_CLIENT_SECRET` (from Aikido → Integrations
  → API, an OAuth client with `rest.api` scope). If absent, the section degrades to status +
  link (no hard failure).

## The report (rendered Markdown; identical in comment + job summary)

1. **Header roll-up:** overall ✅/❌ badge, `<passed>/<total>` checks, total CI wall-clock,
   PR # + title + author.
2. **PR diff stats:** branch, commits, files changed, +additions / −deletions.
3. **Check results table:** each check — status emoji, name, duration, link.
4. **Quality metrics:** SonarCloud quality gate + new-code measures; CodeQL alert count.
5. **Findings** (collapsible `<details>` per source, capped with "and N more…" + link when
   long): CodeQL alerts · SonarCloud issues + hotspots · Aikido open issues · other
   check annotations (build warnings, etc.).
6. **Footer:** link to the run, UTC timestamp.

## Delivery

- **Sticky PR comment:** list issue comments, find one containing marker `<!-- pr-report -->`,
  update it if present else create — so each push refreshes one comment instead of stacking.
- **Job summary:** write the same Markdown to `$GITHUB_STEP_SUMMARY`.

## Edge cases

- **Fork PRs:** `pull_requests[]` may be empty; skip commenting (single-maintainer repo — fine).
- **Poll timeout:** after ~20 min, report with whatever is complete and mark the rest
  "still running" rather than hang.
- **Self-exclusion:** ignore this workflow's own check-run when deciding "all complete."
- **Graceful degradation:** any source that 404s / errors / lacks a secret (CodeQL disabled,
  SonarCloud PR not found, Aikido secret missing) renders "unavailable", never fails the job.
- **Aikido token:** short-lived; fetched once per run.

## Manual prerequisites (one-time)

1. Aikido → **Integrations → API** → create an OAuth client (`rest.api` scope) → copy client
   id + secret.
2. GitHub → repo secrets → add `AIKIDO_API_CLIENT_ID` and `AIKIDO_API_CLIENT_SECRET`.

## Out of scope

- Email delivery (explicitly declined).
- Per-PR-diff scoping of Aikido findings (API is repo-scoped).
- Changes to existing workflows.
