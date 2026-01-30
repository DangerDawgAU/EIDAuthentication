# This script fetches all issues and security hotspots from a public SonarCloud project and saves them to markdown files.

# SonarCloud project key
$projectKey = "DangerDawgAU_EIDAuthentication"
$sonarCloudUrl = "https://sonarcloud.io"

# Create a directory to store the issues and hotspots
$outputDir = "sonarqube_issues"
if (Test-Path -Path $outputDir) {
    Remove-Item -Path $outputDir -Recurse -Force
}
New-Item -ItemType Directory -Path $outputDir

$page = 1
$pageSize = 500
$issues = @()

do {
    # Construct the API URL
    $apiUrl = "$sonarCloudUrl/api/issues/search?componentKeys=$projectKey&statuses=OPEN&p=$page&ps=$pageSize"

    try {
        # Fetch the issues
        $response = Invoke-RestMethod -Uri $apiUrl -Method Get
        $issues += $response.issues
        $total = $response.total
        $fetchedCount = ($page - 1) * $pageSize + $response.issues.Count
        Write-Host "Fetched page $page. Total issues so far: $($issues.Count)/$total"
        $page++
    }
    catch {
        Write-Error "Error fetching issues from SonarCloud: $_"
        break
    }
} while ($fetchedCount -lt $total)

Write-Host "Successfully fetched $($issues.Count) issues."

# Fetch security hotspots
$page = 1
$pageSize = 500
$hotspots = @()

do {
    # Construct the API URL for hotspots
    $apiUrl = "$sonarCloudUrl/api/hotspots/search?projectKey=$projectKey&p=$page&ps=$pageSize"

    try {
        # Fetch the hotspots
        $response = Invoke-RestMethod -Uri $apiUrl -Method Get
        $hotspots += $response.hotspots
        $total = $response.paging.total
        $fetchedCount = ($page - 1) * $pageSize + $response.hotspots.Count
        Write-Host "Fetched hotspot page $page. Total hotspots so far: $($hotspots.Count)/$total"
        $page++
    }
    catch {
        Write-Error "Error fetching hotspots from SonarCloud: $_"
        break
    }
} while ($fetchedCount -lt $total)

Write-Host "Successfully fetched $($hotspots.Count) security hotspots."

# Create individual markdown files for each issue, organized by severity
foreach ($issue in $issues) {
    $severity = $issue.severity
    $severityDir = Join-Path -Path $outputDir -ChildPath "issues\$severity"
    if (-not (Test-Path -Path $severityDir)) {
        New-Item -ItemType Directory -Path $severityDir -Force
    }

    $issueKey = $issue.key
    $issueFile = Join-Path -Path $severityDir -ChildPath "$issueKey.md"
    $issueContent = @"
# $($issue.message)

**Severity:** $($issue.severity)
**Component:** $($issue.component)
**Line:** $($issue.line)
**Status:** $($issue.status)
**Type:** $($issue.type)
**Message:** $($issue.message)
[Link to issue]($($sonarCloudUrl)/project/issues?id=$projectKey&open=$issueKey)
"@
    $issueContent | Out-File -FilePath $issueFile -Encoding utf8
}

Write-Host "Created markdown files for each issue in respective severity folders under $outputDir\issues"

# Create individual markdown files for each security hotspot, organized by severity
foreach ($hotspot in $hotspots) {
    $severity = $hotspot.vulnerabilityProbability
    $severityDir = Join-Path -Path $outputDir -ChildPath "hotspots\$severity"
    if (-not (Test-Path -Path $severityDir)) {
        New-Item -ItemType Directory -Path $severityDir -Force
    }

    $hotspotKey = $hotspot.key
    $hotspotFile = Join-Path -Path $severityDir -ChildPath "$hotspotKey.md"
    $hotspotContent = @"
# $($hotspot.ruleKey)

**Severity:** $($hotspot.vulnerabilityProbability)
**Component:** $($hotspot.component)
**Line:** $($hotspot.line)
**Status:** $($hotspot.status)
**Rule:** $($hotspot.ruleKey)
**Message:** $($hotspot.message)
[Link to hotspot]($($sonarCloudUrl)/project/security_hotspots?id=$projectKey&hotspots=$hotspotKey)
"@
    $hotspotContent | Out-File -FilePath $hotspotFile -Encoding utf8
}

Write-Host "Created markdown files for each security hotspot in respective severity folders under $outputDir\hotspots"
Write-Host "Total: $($issues.Count) issues and $($hotspots.Count) security hotspots processed."
