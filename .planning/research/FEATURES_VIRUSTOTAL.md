# Feature Research: VirusTotal CI/CD Integration

**Domain:** VirusTotal scanning integration for Windows C++ build artifacts
**Researched:** 2026-02-19
**Confidence:** MEDIUM (based on official action documentation and VirusTotal API docs; web search rate-limited)

---

## Executive Summary

This research covers VirusTotal integration features for the EIDAuthentication GitHub Actions workflow. The goal is to scan all build artifacts (binaries, installer, potentially source archives) on push to main branch, provide report URLs in commit comments, and operate in a non-blocking manner (scan failures should not prevent builds).

**Key Findings:**
1. **Primary action identified** - `crazy-max/ghaction-virustotal@v4` is the standard GitHub Action for VirusTotal integration
2. **Free API has rate limits** - 4 requests/minute for public API; must configure `request_rate` accordingly
3. **Commit commenting not built-in** - The action outputs analysis URLs but doesn't automatically comment on commits; requires additional GitHub API integration
4. **Release body updates supported** - Action can automatically append scan results to release notes

---

## Feature Landscape

### Table Stakes (Users Expect These)

Features that are essential for any VirusTotal CI/CD integration.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| **API Key Management** | Security requirement; never hardcode | LOW | Use GitHub Secrets (`secrets.VT_API_KEY`) |
| **File Scanning** | Core functionality | LOW | Action handles upload and scan |
| **Report URL Output** | Users need to view detailed results | LOW | Action outputs `analysis` as `<filename>=<URL>` |
| **Rate Limiting** | Free API has strict limits (4/min) | LOW | Set `request_rate: 4` for public API |
| **Non-Blocking Mode** | Build shouldn't fail on scan issues | LOW | Use `continue-on-error: true` |

### Differentiators (Competitive Advantage)

Features that add value beyond basic scanning.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| **Commit Comment Integration** | Developers see scan results directly in PR/commit | MEDIUM | Requires separate GitHub API step |
| **Release Body Updates** | Release notes automatically include scan links | LOW | Built-in: `update_release_body: true` |
| **Threshold-Based Warnings** | Alert only when detections exceed threshold | MEDIUM | Requires parsing analysis output |
| **Source Archive Scanning** | Full source tarball scanned for supply chain security | LOW | Add source artifact to scan list |
| **Multi-Artifact Scanning** | All binaries + installer scanned together | LOW | Comma-separated file glob |
| **SARIF Output** | Integration with GitHub Code Scanning | MEDIUM | Would require custom conversion |

### Anti-Features (What NOT to Do)

Patterns that seem beneficial but cause problems.

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| **Block builds on detections** | Security enforcement | Legitimate software often has false positives; developer friction | Non-blocking warnings with manual review |
| **Scan intermediate build artifacts** | More coverage | Wastes API quota on transient files; noise from debug symbols | Scan final release artifacts only |
| **Scan on every push** | Maximum coverage | Rapidly consumes API quota; unnecessary for draft commits | Scan on main branch push only |
| **Fail on API errors** | Visibility | Network issues shouldn't block releases | Log errors, continue build |
| **Real-time scanning during build** | Immediate feedback | Adds latency to build; not supported by VirusTotal | Post-build async scanning |

---

## Feature Dependencies

```
API Key Storage (GitHub Secret)
    └──required──> File Scanning
                       ├──produces──> Report URL Output
                       │                   └──required──> Commit Comment Integration
                       │                   └──required──> Release Body Updates
                       └──requires──> Rate Limiting

Build Artifacts
    └──required──> Multi-Artifact Scanning
                       └──produces──> Analysis Output (JSON)
                                            └──enables──> Threshold-Based Warnings
```

### Dependency Notes

- **API Key Storage required for File Scanning:** VirusTotal requires authentication; GitHub Secrets provide secure storage
- **Report URL Output required for Commit Comments:** The action outputs URLs that must be captured and posted via GitHub API
- **Rate Limiting required for Multi-Artifact Scanning:** Scanning multiple files needs rate limiting to avoid API 429 errors
- **Analysis Output enables Threshold Warnings:** Parsing the output allows detection count extraction for conditional warnings

---

## MVP Definition

### Launch With (v1)

Minimum viable VirusTotal integration - validates the concept with minimal complexity.

- [x] **API Key Secret** - Store `VT_API_KEY` in repository secrets
- [x] **Binary Artifact Scanning** - Scan compiled DLLs/EXEs after build
- [x] **Installer Scanning** - Scan NSIS installer executable
- [x] **Non-Blocking Execution** - Use `continue-on-error: true`
- [x] **Workflow Log Output** - Analysis URLs visible in workflow logs
- [x] **Rate Limiting** - Set `request_rate: 4` for free API

### Add After Validation (v1.x)

Features to add once basic scanning is working.

- [ ] **Commit Comment Integration** - Post scan results as commit comment (trigger: developer request or after stability)
- [ ] **Source Archive Scanning** - Include source tarball in scan (trigger: after binary scanning stable)
- [ ] **Release Body Updates** - Append scan links to release notes (trigger: first release after integration)
- [ ] **Workflow Summary** - Post scan summary to GitHub Actions summary (trigger: developer experience improvement)

### Future Consideration (v2+)

Features to defer until integration is proven.

- [ ] **Threshold-Based Blocking** - Block releases with high detection counts (needs baseline data)
- [ ] **SARIF Integration** - Convert VT results to SARIF for Code Scanning (needs custom script)
- [ ] **Scheduled Re-scanning** - Periodically re-scan released artifacts for new detections (needs separate workflow)
- [ ] **Private API Upgrade** - Move to premium API for higher rate limits (requires budget)

---

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| API Key Secret | HIGH | LOW | P1 |
| Binary Artifact Scanning | HIGH | LOW | P1 |
| Installer Scanning | HIGH | LOW | P1 |
| Non-Blocking Execution | HIGH | LOW | P1 |
| Rate Limiting | HIGH | LOW | P1 |
| Workflow Log Output | MEDIUM | LOW | P1 |
| Commit Comment Integration | MEDIUM | MEDIUM | P2 |
| Release Body Updates | MEDIUM | LOW | P2 |
| Source Archive Scanning | LOW | LOW | P3 |
| Threshold-Based Warnings | MEDIUM | MEDIUM | P3 |
| SARIF Integration | LOW | HIGH | P4 |

**Priority key:**
- P1: Must have for initial integration
- P2: Should have, add when possible
- P3: Nice to have, future consideration
- P4: Defer unless specifically needed

---

## Implementation Details

### Primary Action: crazy-max/ghaction-virustotal@v4

**Documentation:** https://github.com/crazy-max/ghaction-virustotal

**Key Inputs:**
| Input | Required | Description |
|-------|----------|-------------|
| `vt_api_key` | Yes | VirusTotal API key |
| `files` | Yes | Files to scan (glob pattern or comma-separated) |
| `request_rate` | No | Rate limit (use 4 for free public API) |
| `update_release_body` | No | Append results to release body |
| `github_token` | No | Token for release body updates |
| `vt_monitor` | No | Use VirusTotal Monitor (premium) |
| `monitor_path` | No | Path in Monitor (premium) |

**Key Output:**
| Output | Description |
|--------|-------------|
| `analysis` | Analysis results as `<filename>=<analysisURL>` (comma-separated if multiple) |

### Example Workflow Pattern

```yaml
- name: Scan artifacts with VirusTotal
  id: virustotal
  if: github.event_name == 'push' && github.ref == 'refs/heads/main'
  continue-on-error: true
  uses: crazy-max/ghaction-virustotal@v4
  with:
    vt_api_key: ${{ secrets.VT_API_KEY }}
    files: |
      Installer/EIDInstallx64.exe,
      x64/Release/*.dll,
      x64/Release/*.exe
    request_rate: 4

- name: Log VirusTotal results
  if: steps.virustotal.outcome != 'skipped'
  run: |
    echo "VirusTotal Analysis Results:"
    echo "${{ steps.virustotal.outputs.analysis }}"
```

### Commit Comment Pattern (P2 Feature)

```yaml
- name: Comment scan results on commit
  if: steps.virustotal.outputs.analysis != ''
  uses: actions/github-script@v7
  with:
    script: |
      const analysis = '${{ steps.virustotal.outputs.analysis }}';
      const body = `## VirusTotal Scan Results\n\n${analysis.split(',').map(r => `- ${r}`).join('\n')}`;
      await github.rest.repos.createCommitComment({
        owner: context.repo.owner,
        repo: context.repo.repo,
        commit_sha: context.sha,
        body: body
      });
```

---

## Rate Limiting Considerations

**Free Public API Limits:**
- 4 requests per minute
- 500 KB file size limit (for immediate scan)
- Larger files queued (may take minutes)

**Mitigation Strategies:**
1. Set `request_rate: 4` to stay within limits
2. Scan only final artifacts, not intermediate files
3. Consider artifact size - large files may be queued
4. For multiple artifacts, expect 15+ seconds between scans

**Premium API (Future):**
- Higher rate limits
- Priority queue for large files
- VirusTotal Monitor for private samples

---

## Competitor Feature Analysis

| Feature | crazy-max/ghaction-virustotal | EnricoMi/publish-unit-test-result-action | Custom Script |
|---------|-------------------------------|------------------------------------------|---------------|
| File Upload | Yes | No | Yes |
| Analysis URL Output | Yes | No | Yes |
| Release Integration | Yes | No | Yes |
| Commit Comments | Manual | Built-in | Custom |
| SARIF Output | No | No | Custom |
| Maintenance | Active | N/A | Self |
| Ease of Use | HIGH | N/A | LOW |

**Recommendation:** Use `crazy-max/ghaction-virustotal@v4` for scanning; add `actions/github-script@v7` for commit commenting.

---

## Sources

- [crazy-max/ghaction-virustotal GitHub Action](https://github.com/crazy-max/ghaction-virustotal) - HIGH confidence (official documentation)
- [VirusTotal API v3 Overview](https://developers.virustotal.com/reference/overview) - HIGH confidence (official docs)
- [VirusTotal API Rate Limits](https://developers.virustotal.com/reference/public-vs-private-api) - MEDIUM confidence (retrieved from API docs)
- [GitHub Actions continue-on-error](https://docs.github.com/en/actions/using-workflows/workflow-syntax-for-github-actions#jobsjob_idstepscontinue-on-error) - HIGH confidence (official docs)
- [actions/github-script](https://github.com/actions/github-script) - HIGH confidence (official action)

---

*Feature research for: VirusTotal CI/CD integration*
*Researched: 2026-02-19*
