# Stack Research: VirusTotal CI/CD Integration

**Domain:** VirusTotal scanning for Windows authentication package releases
**Researched:** 2026-02-19
**Confidence:** HIGH

## Executive Summary

Adding automated malware scanning via VirusTotal API to the GitHub Actions workflow requires **one primary addition**: the `crazy-max/ghaction-virustotal@v4` GitHub Action. This is the de facto standard for VirusTotal integration in GitHub Actions with active maintenance, built-in rate limiting, and comprehensive feature support.

**Minimal Stack Impact:**
- No new build dependencies required
- No changes to existing C++23 build system
- Single GitHub Action addition to workflow
- One GitHub secret (`VT_API_KEY`) required

---

## Recommended Stack

### Core GitHub Action

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| `crazy-max/ghaction-virustotal` | v4 | VirusTotal API integration | Most maintained action (active CI/testing), supports local files + release assets + VT Monitor, built-in rate limiting for free API tier |

### Supporting Actions (Already in Project)

| Technology | Version | Purpose | Notes |
|------------|---------|---------|-------|
| `actions/checkout` | v4 | Repository checkout | Already used in existing workflows |
| `actions/download-artifact` | v4 | Retrieve build artifacts | Required to access compiled binaries |
| `actions/upload-artifact` | v4 | Persist build outputs | Required to pass binaries to scan job |

### External Service

| Service | Tier | Purpose | Why |
|---------|------|---------|-----|
| VirusTotal API v3 | Free (Public) | Malware scanning backend | 70+ antivirus engines, free for non-commercial use, comprehensive analysis reports |

---

## Configuration

### Required GitHub Secret

```
VT_API_KEY  # VirusTotal API key (get from virustotal.com while signed in)
```

**Setup:**
1. Sign in to VirusTotal
2. Navigate to your API key view
3. Copy the API key
4. Add to repository Settings > Secrets and variables > Actions > New repository secret

### Recommended Workflow Configuration

```yaml
# .github/workflows/virustotal.yml
name: VirusTotal Scan

on:
  release:
    types: [published]
  workflow_dispatch:

permissions:
  contents: write  # Required for update_release_body

jobs:
  virustotal:
    runs-on: ubuntu-latest
    steps:
      - name: VirusTotal Scan
        uses: crazy-max/ghaction-virustotal@v4
        with:
          vt_api_key: ${{ secrets.VT_API_KEY }}
          request_rate: 4  # Free tier: 4 requests/minute
          update_release_body: true
          files: |
            .exe$
            .dll$
```

### Key Input Parameters

| Parameter | Required | Purpose | Recommended Value |
|-----------|----------|---------|-------------------|
| `vt_api_key` | YES | VirusTotal API authentication | `${{ secrets.VT_API_KEY }}` |
| `files` | YES | Files/patterns to scan | `.exe$` and `.dll$` for release assets |
| `request_rate` | NO | Rate limiting (requests/min) | `4` for free tier, `0` to disable |
| `update_release_body` | NO | Add scan links to release notes | `true` for public transparency |
| `github_token` | Conditional | GitHub API access | Required if `update_release_body: true` |

---

## Installation

No package installation required. The action is referenced directly from GitHub Marketplace in the workflow YAML.

```yaml
# Single line addition to workflow
uses: crazy-max/ghaction-virustotal@v4
```

---

## Integration with EIDAuthentication Build

### Artifacts to Scan

Based on the project's 7 compiled outputs plus NSIS installer:

| Project | Output Type | Pattern |
|---------|-------------|----------|
| EIDCredentialProvider | DLL | `*.dll` |
| EIDConfigurationWizard | EXE | `*.exe` |
| EIDConfigurationWizardElevated | EXE | `*.exe` |
| EIDLogManager | DLL | `*.dll` |
| EIDPasswordChangeNotification | DLL | `*.dll` |
| EIDCardLibrary | DLL | `*.dll` |
| EIDAuthenticationPackage | DLL | `*.dll` |
| NSIS Installer | EXE | `*.exe` |

### Workflow Integration Options

**Option A: Separate Workflow (Recommended)**

```yaml
# .github/workflows/virustotal.yml
# Triggers on release events only
# Does not block release if scan fails
# Adds scan links to release notes
```

**Option B: Job in Existing Release Workflow**

```yaml
# Add to existing release workflow
# Run after artifact upload
# Add as final step before release publish
```

**Option C: Build + Scan in Same Workflow**

```yaml
# Upload artifacts from build job
# Download in virustotal job (needs: build)
# Scan all compiled outputs
```

---

## What NOT to Do

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| Store `VT_API_KEY` in code | Security exposure | GitHub repository secrets |
| Scan on every push | Wastes API quota | Scan on releases only |
| Skip `request_rate` on free tier | 429 errors, failed scans | Always set `request_rate: 4` |
| Use VirusTotal API v2 | Deprecated, fewer features | API v3 (default with ghaction-virustotal@v4) |
| Commercial use with free tier | Violates ToS | VirusTotal Premium/Enterprise |

---

## Alternatives Considered

| Recommended | Alternative | Why Not |
|-------------|-------------|---------|
| `crazy-max/ghaction-virustotal@v4` | Custom curl script | Reinventing the wheel, no rate limiting, more maintenance |
| `crazy-max/ghaction-virustotal@v4` | `cedrickring/virus-total-action` | Less maintained, fewer features |
| `crazy-max/ghaction-virustotal@v4` | MetaDefender API | Different service, requires separate API key |
| VirusTotal Free | VirusTotal Premium | Overkill for open source project; upgrade if rate limits hit |

---

## API Rate Limits

| Tier | Requests/Minute | Daily Quota | Use Case |
|------|-----------------|-------------|----------|
| Free (Public) | 4 | ~5,760 | Non-commercial, open source |
| Premium | Higher | Higher | Professional use |
| Enterprise | Custom | Custom | Commercial redistribution |

**For EIDAuthentication:** Free tier is sufficient. With ~8 artifacts per release and infrequent releases, the 4 req/min limit will not be a bottleneck.

---

## Expected Output

When a release is published, the action will:

1. Upload each matching file to VirusTotal
2. Wait for analysis to complete
3. Append scan results to release body (if `update_release_body: true`)
4. Output analysis URLs for downstream use

**Release Notes Example:**
```markdown
## VirusTotal Analysis

- EIDCredentialProvider.dll: [Scan Results](https://virustotal.com/...)
- EIDConfigurationWizard.exe: [Scan Results](https://virustotal.com/...)
- EIDInstaller.exe: [Scan Results](https://virustotal.com/...)
```

---

## Sources

- [GitHub - crazy-max/ghaction-virustotal](https://github.com/crazy-max/ghaction-virustotal) - Official repository with v4 documentation (HIGH confidence)
- [GitHub Marketplace - VirusTotal GitHub Action](https://github.com/marketplace/actions/virustotal-github-action) - Marketplace listing (HIGH confidence)
- [VirusTotal API v3 Documentation](https://developers.virustotal.com/reference/overview) - Official API reference (HIGH confidence)

---

## Summary

**Single Addition Required:**
```yaml
- name: VirusTotal Scan
  uses: crazy-max/ghaction-virustotal@v4
  with:
    vt_api_key: ${{ secrets.VT_API_KEY }}
    request_rate: 4
    update_release_body: true
    files: |
      .exe$
      .dll$
```

**Prerequisites:**
1. Add `VT_API_KEY` secret to repository
2. Create `.github/workflows/virustotal.yml` with above configuration
3. Ensure release artifacts include `.exe` and `.dll` files

**No Build System Changes Required.**
