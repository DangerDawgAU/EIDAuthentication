# Pitfalls Research

**Domain:** VirusTotal CI/CD Integration for Security Software
**Project:** EIDAuthentication - Windows Credential Provider with LSASS interaction
**Researched:** 2026-02-19
**Confidence:** MEDIUM (based on official VirusTotal documentation, GitHub Action docs, and security software scanning patterns; LOW for specific false positive rates which vary by vendor)

---

## Executive Summary

Adding VirusTotal scanning to the EIDAuthentication CI/CD pipeline requires careful consideration of API rate limits, false positive handling for security-sensitive software, and secure credential management. The project's nature as a Windows credential provider that interacts with LSASS makes it particularly susceptible to heuristic antivirus detections that flag legitimate security software as potentially malicious.

---

## Critical Pitfalls

Mistakes that cause CI/CD failures, security exposures, or production incidents.

### Pitfall 1: API Rate Limit Exhaustion

**What goes wrong:**
The VirusTotal Public API enforces strict rate limits (4 requests/minute, 500 requests/day). Exceeding these limits causes HTTP 429 (TooManyRequests) errors, which silently fail CI/CD pipelines or cause indefinite hangs when retry logic is not implemented correctly.

**Why it happens:**
- Multiple concurrent CI/CD jobs trigger simultaneous scans
- Release workflows scan multiple artifacts without rate limiting
- Retry loops without backoff compound the problem
- Public API quota is shared across all users of the API key

**How to avoid:**
1. Use the `request_rate` parameter in crazy-max/ghaction-virustotal action
2. Implement the action's built-in rate limiting (it handles 429 responses automatically)
3. Consider VirusTotal Premium API for higher limits if scanning frequently
4. Only scan release artifacts, not every commit

```yaml
- name: VirusTotal Scan
  uses: crazy-max/ghaction-virustotal@v4
  with:
    vt_api_key: ${{ secrets.VT_API_KEY }}
    files: ./build/EIDCredentialProvider.dll
    request_rate: 4  # Stay within 4 requests/minute limit
```

**Warning signs:**
- CI/CD jobs timing out during VirusTotal step
- HTTP 429 responses in workflow logs
- "QuotaExceededError" in action output
- Scans completing sometimes but failing other times

**Phase to address:** Initial VirusTotal integration phase

---

### Pitfall 2: False Positive Blocking for Security Software

**What goes wrong:**
EIDAuthentication is a credential provider that:
- Interacts with LSASS (Windows security subsystem)
- Handles PIN codes and smart card authentication
- Loads into the Windows logon process
- Uses cryptographic operations

These behaviors trigger heuristic detections in multiple antivirus engines. A false positive detection can block releases, trigger security alerts, or damage the project's reputation.

**Why it happens:**
Antivirus engines use behavioral heuristics that flag:
- LSASS interaction (often associated with credential theft malware)
- Smart card access (sometimes associated with card skimming)
- Memory operations in security contexts
- Unsigned or newly-seen binaries

**How to avoid:**
1. **Code signing is essential** - Sign all DLLs with a valid Authenticode certificate
2. **Do not fail CI on detections** - Use the scan for awareness, not gating
3. **Monitor trends** - Track which engines flag and look for patterns
4. **Document expected detections** - Create internal baseline of expected false positives
5. **Consider VirusTotal Monitor** - For software publishers, this allows pre-allowlisting

```yaml
- name: VirusTotal Scan
  id: vt-scan
  uses: crazy-max/ghaction-virustotal@v4
  with:
    vt_api_key: ${{ secrets.VT_API_KEY }}
    files: ./build/*.dll
    update_release_body: true  # Adds results to release notes
  continue-on-error: true  # Don't fail the release on detections

- name: Check scan results
  if: steps.vt-scan.outcome == 'success'
  run: |
    # Parse results and alert if unexpected engines flag
    echo "Scan completed - review results in release notes"
```

**Warning signs:**
- New engines flagging that didn't before
- Detection names containing "PUP", "Riskware", "HackTool"
- Dramatic increase in detection count from previous scan
- Specific engine consistently flaging

**Phase to address:** Initial integration + ongoing monitoring

---

### Pitfall 3: Exposed API Keys in Repository

**What goes wrong:**
Hardcoding VirusTotal API keys or accidentally committing them to the repository exposes the key to abuse. This can lead to:
- API quota exhaustion by unauthorized users
- Account suspension by VirusTotal
- Security audit findings

**Why it happens:**
- Testing with hardcoded keys and forgetting to remove
- Key accidentally included in committed .env file
- Key visible in workflow logs when not properly masked
- Fork of repository includes original keys

**How to avoid:**
1. **Always use GitHub Secrets** - Never hardcode API keys
2. **Use repository or environment secrets** - Not organization-wide if not needed
3. **Restrict secret access** - Use environment protection rules for production
4. **Rotate keys periodically** - Especially after any potential exposure
5. **Audit workflow files** - Ensure no secrets in logs or outputs

```yaml
# CORRECT - Using GitHub Secret
- name: VirusTotal Scan
  uses: crazy-max/ghaction-virustotal@v4
  with:
    vt_api_key: ${{ secrets.VT_API_KEY }}  # From repository secrets

# WRONG - Never do this
# vt_api_key: "abc123def456..."  # Hardcoded key exposed in repo
```

**Warning signs:**
- API key visible in plain text anywhere in repository
- Unexpected API usage in VirusTotal account dashboard
- API quota depleted without corresponding CI/CD runs
- Security scanner flagging exposed credentials

**Phase to address:** Initial setup phase

---

### Pitfall 4: Asynchronous Analysis Timeout

**What goes wrong:**
VirusTotal analysis is asynchronous. The API returns immediately with an analysis ID, but results may take minutes to complete. CI/CD workflows that don't handle this properly will:
- Report success before analysis completes
- Timeout waiting for results
- Return incomplete scan information

**Why it happens:**
- Large files take longer to analyze
- VirusTotal queue times vary (can be 1-10+ minutes)
- Default GitHub Actions timeout (6 hours) is usually sufficient, but shorter job timeouts may not be
- Not using the action's built-in polling mechanism

**How to avoid:**
1. Use the crazy-max/ghaction-virustotal action which handles polling automatically
2. Set appropriate timeout for the job (at least 15-30 minutes for large files)
3. For manual API usage, implement polling loop with exponential backoff
4. Consider `vt_monitor` feature for pre-release scanning

```yaml
jobs:
  virus-scan:
    runs-on: ubuntu-latest
    timeout-minutes: 30  # Allow time for analysis
    steps:
      - name: VirusTotal Scan
        uses: crazy-max/ghaction-virustotal@v4
        with:
          vt_api_key: ${{ secrets.VT_API_KEY }}
          files: ./build/*.dll
          # Action handles polling automatically
```

**Warning signs:**
- Workflow shows "success" but no scan results in output
- Analysis ID returned but no verdict
- Intermittent failures on larger files
- Logs show "analysis in progress" until timeout

**Phase to address:** Initial integration phase

---

### Pitfall 5: Scan Results Not Integrated with Release Process

**What goes wrong:**
Running VirusTotal scans but not surfacing results where users/stakeholders can see them means:
- Security-conscious users don't see the clean scan
- False positive issues can't be investigated retroactively
- No audit trail of scan results over time
- Releases proceed without security review

**Why it happens:**
- Scan runs but output is only in workflow logs
- Results not attached to GitHub Release
- No notification when detections occur
- Scans run on non-release builds and get lost

**How to avoid:**
1. **Use `update_release_body: true`** - Automatically adds scan results to release notes
2. **Run scans on release events, not every push**
3. **Store scan URLs as artifacts** for non-release builds
4. **Add workflow step to comment scan results on PRs** (optional)

```yaml
# Scan on release
on:
  release:
    types: [published]

jobs:
  scan:
    runs-on: ubuntu-latest
    steps:
      - name: VirusTotal Scan
        uses: crazy-max/ghaction-virustotal@v4
        with:
          vt_api_key: ${{ secrets.VT_API_KEY }}
          files: ./build/EIDCredentialProvider.dll
          update_release_body: true  # Adds results to release notes
```

**Warning signs:**
- Release notes don't mention VirusTotal
- Users asking "has this been scanned for viruses?"
- No record of what scan results were at release time
- Manual step required to find scan results

**Phase to address:** Release integration phase

---

### Pitfall 6: Scanning Wrong Artifacts

**What goes wrong:**
Scanning source code, intermediate build files, or wrong configuration results in:
- False sense of security (scanned source, not binary)
- Scanning files users never receive
- Missing the actual deployed artifacts
- Wasting API quota on irrelevant files

**Why it happens:**
- Glob pattern matches more than intended
- Build outputs in unexpected locations
- Forgetting to build before scanning
- Scanning test fixtures instead of release binaries

**How to avoid:**
1. **Only scan release artifacts** - The actual DLLs users will install
2. **Run after build step** - Ensure artifacts exist before scanning
3. **Use explicit file paths** - Not broad glob patterns
4. **Include in release workflow** - Not general CI

```yaml
jobs:
  build:
    runs-on: windows-latest
    outputs:
      artifact-path: ./build/x64/Release
    steps:
      - name: Build
        run: msbuild EIDCredentialProvider.sln /p:Configuration=Release /p:Platform=x64

  scan:
    needs: build
    runs-on: ubuntu-latest
    steps:
      - name: Download artifacts
        uses: actions/download-artifact@v4
        with:
          name: release-binaries

      - name: VirusTotal Scan
        uses: crazy-max/ghaction-virustotal@v4
        with:
          vt_api_key: ${{ secrets.VT_API_KEY }}
          files: |
            ./EIDCredentialProvider.dll
            ./EIDCardLibrary.dll
          # Explicit paths to release artifacts only
```

**Warning signs:**
- Scanning *.dll matches test DLLs
- Source .cpp files in scan results
- Scan reports 0 detections because scanned wrong files
- File sizes in scan don't match release sizes

**Phase to address:** Initial integration phase

---

## Moderate Pitfalls

### Pitfall 7: Not Handling VirusTotal Service Outages

**What goes wrong:**
VirusTotal occasionally experiences outages or degraded performance. CI/CD pipelines that require VirusTotal to succeed will block releases during outages.

**How to avoid:**
1. Use `continue-on-error: true` for non-blocking scans
2. Have a manual bypass process for critical releases
3. Monitor VirusTotal status page
4. Consider VirusTotal Premium with SLA guarantees

**Warning signs:**
- Consistent timeouts across multiple runs
- HTTP 503 errors in logs
- VirusTotal status page shows incidents

**Phase to address:** Production hardening phase

---

### Pitfall 8: Using Deprecated API v2 Instead of v3

**What goes wrong:**
VirusTotal API v2 is deprecated (though not yet removed). Using v2:
- May stop working without notice
- Has different rate limits and features
- Less detailed analysis results
- No Monitor feature support

**How to avoid:**
1. Ensure action version uses API v3 (crazy-max/ghaction-virustotal@v4 uses v3)
2. If using vt-py directly, specify v3 endpoints
3. Monitor VirusTotal deprecation announcements

**Phase to address:** Initial integration phase

---

### Pitfall 9: Not Setting Up VirusTotal Monitor for Publisher

**What goes wrong:**
Without VirusTotal Monitor, every release is a new unknown to antivirus engines. This increases false positive rates and delays releases.

**How to avoid:**
For established publishers with consistent release patterns:
1. Sign up for VirusTotal Monitor (separate from standard API)
2. Submit software for allowlisting consideration
3. Maintain consistent code signing certificate
4. Build detection history over time

**Note:** Monitor requires VirusTotal Premium subscription.

**Phase to address:** Post-MVP optimization (if releasing frequently)

---

## Minor Pitfalls

### Pitfall 10: Verbose Output in Workflow Logs

**What goes wrong:**
Full scan output with all 70+ engine results creates verbose, hard-to-read workflow logs.

**How to avoid:**
- Use the action's summary output instead of full JSON
- Parse results to show only detections, not clean engines
- Link to VirusTotal web interface for full details

**Phase to address:** Polish phase

---

### Pitfall 11: Not Documenting Expected False Positives

**What goes wrong:**
When a new detection appears, there's no baseline to compare against to determine if it's expected or concerning.

**How to avoid:**
- Document which engines typically flag security software
- Track detection trends over time
- Create baseline documentation after first clean scan

**Phase to address:** First release with scanning

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Skip scanning for patches | Faster releases | No detection history | Only for critical security patches |
| Use continue-on-error always | Never blocks releases | Real threats may go unnoticed | Only if scanning is informational |
| Scan source instead of binary | Faster scan | False sense of security | Never |
| Share API key across projects | Simpler setup | Quota contention, audit issues | Only for personal/low-volume projects |
| No code signing | Saves certificate cost | Guaranteed false positives | Never for security software |

---

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| GitHub Actions | Hardcoding API key | Use `secrets.VT_API_KEY` |
| Release workflow | Scanning before build | Build first, then scan |
| Multiple artifacts | Separate scan jobs | Use single job with file list |
| Private repo | Public API key scope | Use repo-specific secret |
| Fork workflow | Copying secrets | Each fork needs own API key |

---

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Concurrent scans | 429 errors | Use sequential scans or rate limiting | >1 PR merged simultaneously |
| Large file scan | Timeouts | Increase job timeout, consider file size limits | Files >100MB |
| Queue backlog | Long wait times | Scan off-peak, use Premium API | VirusTotal heavy usage periods |
| Daily quota exceeded | All scans fail | Track usage, implement job limits | >500 scans/day (unlikely for releases) |

---

## Security Mistakes

| Mistake | Risk | Prevention |
|---------|------|------------|
| API key in workflow file | Key exposure, quota abuse | Use GitHub Secrets |
| API key in fork | Unauthorized usage | Document fork setup process |
| Scan results expose URLs | Public access to private releases | Ensure release visibility matches intent |
| No key rotation | Long-term exposure risk | Rotate keys quarterly or after exposure |
| Bypassing scan for "trusted" commits | Supply chain attack vector | Never bypass, always scan |

---

## Security Software Specific Considerations

For EIDAuthentication specifically, expect these behaviors to trigger detections:

| Behavior | Why It Triggers | Expected Detection Type |
|----------|-----------------|------------------------|
| LSASS interaction | Credential theft malware pattern | "Riskware", "PUP", behavior-based |
| Smart card access | Card skimming pattern | "HackTool", "PUP" |
| PIN handling | Keylogging pattern | Behavior-based |
| Memory operations in logon context | Process injection pattern | "Trojan", behavior-based |
| Unsigned binary | No publisher verification | "Unknown", heuristic |

**Mitigation strategies:**
1. **Code sign everything** - Most important factor in reducing false positives
2. **Build reputation over time** - Same certificate, consistent releases
3. **Document expected detections** - Create baseline for comparison
4. **Engage with antivirus vendors** - Report false positives for allowlisting

---

## "Looks Done But Isn't" Checklist

Things that appear complete but are missing critical pieces.

- [ ] **Workflow runs:** But no VirusTotal results visible - verify `update_release_body: true`
- [ ] **Scan succeeds:** But scanned wrong files - check file paths in scan output
- [ ] **No detections:** But scan was skipped - verify scan step actually ran
- [ ] **API key configured:** But workflow fails - verify secret name matches exactly
- [ ] **Results in release notes:** But link is broken - verify release exists and is public
- [ ] **Scan runs on PR:** But PR from fork fails - forks don't have access to secrets

---

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Exposed API key | MEDIUM | Generate new key, update secret, audit usage |
| False positive blocking release | LOW | `continue-on-error: true`, document detection, proceed |
| Rate limit exceeded | LOW | Wait 1 minute (per-minute limit) or until next day (daily) |
| Wrong files scanned | LOW | Fix file paths, re-run workflow |
| VirusTotal outage | LOW | Wait for service recovery or bypass temporarily |
| No code signing false positives | HIGH | Obtain certificate, sign, resubmit to VirusTotal |

---

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| API rate limit exhaustion | Initial integration | Monitor workflow logs for 429 errors |
| False positive blocking | Initial integration + ongoing | `continue-on-error: true`, baseline documentation |
| Exposed API keys | Setup phase | Secret scanning, key rotation |
| Async analysis timeout | Initial integration | Set job timeout, use action's polling |
| Results not in release | Release integration | Verify release notes contain scan link |
| Scanning wrong artifacts | Initial integration | Compare scanned files to release assets |
| Service outages | Production hardening | `continue-on-error`, bypass process |
| Deprecated API | Initial integration | Verify action uses v3 |
| No Monitor setup | Post-MVP (optional) | Monitor subscription if releasing frequently |

---

## Sources

### Official Documentation (HIGH confidence)
- VirusTotal API v3 Overview: https://docs.virustotal.com/reference/overview
- VirusTotal Error Codes: https://docs.virustotal.com/reference/errors
- GitHub Actions Secrets: https://docs.github.com/en/actions/security-guides/using-secrets-in-github-actions

### GitHub Action Documentation (HIGH confidence)
- crazy-max/ghaction-virustotal: https://github.com/crazy-max/ghaction-virustotal
- Action marketplace entry: https://github.com/marketplace/actions/virustotal-github-action

### Community Knowledge (MEDIUM confidence)
- VirusTotal Public vs Premium API: https://docs.virustotal.com/reference/public-vs-premium
- vt-py Python library: https://github.com/VirusTotal/vt-py

### Security Software Scanning Patterns (MEDIUM confidence)
- General knowledge of how antivirus heuristics flag credential providers
- Common false positive patterns for LSASS-interacting software
- Code signing impact on detection rates (industry standard knowledge)

---

## Appendix: Quick Reference - Action Configuration

```yaml
# Recommended configuration for EIDAuthentication
- name: VirusTotal Scan
  uses: crazy-max/ghaction-virustotal@v4
  with:
    vt_api_key: ${{ secrets.VT_API_KEY }}
    files: |
      ./build/EIDCredentialProvider.dll
      ./build/EIDCardLibrary.dll
    request_rate: 4           # Public API limit
    update_release_body: true # Add to release notes
  continue-on-error: true     # Don't block releases on detections
```

### Key Parameters

| Parameter | Value | Reason |
|-----------|-------|--------|
| `vt_api_key` | From secrets | Never hardcode |
| `files` | Explicit paths | Avoid scanning wrong artifacts |
| `request_rate` | 4 | Stay within public API limit |
| `update_release_body` | true | Surface results to users |
| `continue-on-error` | true | Don't block on false positives |

---

*Pitfalls research for: VirusTotal CI/CD Integration for Security Software*
*Project: EIDAuthentication - Windows Credential Provider*
*Researched: 2026-02-19*
