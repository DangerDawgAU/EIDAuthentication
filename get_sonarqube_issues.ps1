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
    # Construct the API URL - only fetch open issues (excluding RESOLVED and CLOSED)
    $apiUrl = "$sonarCloudUrl/api/issues/search?componentKeys=$projectKey&statuses=OPEN,CONFIRMED,REOPENED&p=$page&ps=$pageSize"

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
    # Construct the API URL for hotspots - filter by TO_REVIEW status to exclude remediated/closed hotspots
    $apiUrl = "$sonarCloudUrl/api/hotspots/search?projectKey=$projectKey&statuses=TO_REVIEW&p=$page&ps=$pageSize"

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

# Filter out any hotspots with REVIEWED status
$hotspots = $hotspots | Where-Object { $_.status -ne 'REVIEWED' }

Write-Host "Successfully fetched $($hotspots.Count) security hotspots (excluding reviewed/remediated/closed)."

# Create individual markdown files for each issue, organized by severity and category
# Map SonarQube severities to user-friendly categories
$severityMap = @{
    'BLOCKER' = 'Blocker'
    'CRITICAL' = 'High'
    'MAJOR' = 'Medium'
    'MINOR' = 'Low'
    'INFO' = 'Info'
}

# Map SonarQube issue types to categories
$categoryMap = @{
    'VULNERABILITY' = 'Security'
    'BUG' = 'Reliability'
    'CODE_SMELL' = 'Maintainability'
}

foreach ($issue in $issues) {
    $originalSeverity = $issue.severity
    $mappedSeverity = $severityMap[$originalSeverity]
    if (-not $mappedSeverity) {
        $mappedSeverity = $originalSeverity
    }

    $issueType = $issue.type
    $mappedCategory = $categoryMap[$issueType]
    if (-not $mappedCategory) {
        $mappedCategory = $issueType
    }

    $severityDir = Join-Path -Path $outputDir -ChildPath "issues\$mappedSeverity"
    if (-not (Test-Path -Path $severityDir)) {
        New-Item -ItemType Directory -Path $severityDir -Force
    }

    $categoryDir = Join-Path -Path $severityDir -ChildPath $mappedCategory
    if (-not (Test-Path -Path $categoryDir)) {
        New-Item -ItemType Directory -Path $categoryDir -Force
    }

    $issueKey = $issue.key
    $issueFile = Join-Path -Path $categoryDir -ChildPath "$issueKey.md"
    $issueContent = @"
# $($issue.message)

**Severity:** $mappedSeverity
**Category:** $mappedCategory
**Component:** $($issue.component)
**Line:** $($issue.line)
**Status:** $($issue.status)
**Type:** $($issue.type)
**Message:** $($issue.message)
[Link to issue]($($sonarCloudUrl)/project/issues?id=$projectKey&open=$issueKey)
"@
    $issueContent | Out-File -FilePath $issueFile -Encoding utf8
}

Write-Host "Created markdown files for each issue in respective severity and category folders under $outputDir\issues"

# Create individual markdown files for each security hotspot, organized by severity and category
# Map SonarQube hotspot vulnerability probabilities to user-friendly categories
$hotspotSeverityMap = @{
    'HIGH' = 'High'
    'MEDIUM' = 'Medium'
    'LOW' = 'Low'
}

# All security hotspots belong to the Security category
$hotspotCategory = 'Security'

foreach ($hotspot in $hotspots) {
    $originalSeverity = $hotspot.vulnerabilityProbability
    $mappedSeverity = $hotspotSeverityMap[$originalSeverity]
    if (-not $mappedSeverity) {
        $mappedSeverity = $originalSeverity
    }

    $severityDir = Join-Path -Path $outputDir -ChildPath "hotspots\$mappedSeverity"
    if (-not (Test-Path -Path $severityDir)) {
        New-Item -ItemType Directory -Path $severityDir -Force
    }

    $categoryDir = Join-Path -Path $severityDir -ChildPath $hotspotCategory
    if (-not (Test-Path -Path $categoryDir)) {
        New-Item -ItemType Directory -Path $categoryDir -Force
    }

    $hotspotKey = $hotspot.key
    $hotspotFile = Join-Path -Path $categoryDir -ChildPath "$hotspotKey.md"
    $hotspotContent = @"
# $($hotspot.ruleKey)

**Severity:** $mappedSeverity
**Category:** $hotspotCategory
**Component:** $($hotspot.component)
**Line:** $($hotspot.line)
**Status:** $($hotspot.status)
**Rule:** $($hotspot.ruleKey)
**Message:** $($hotspot.message)
[Link to hotspot]($($sonarCloudUrl)/project/security_hotspots?id=$projectKey&hotspots=$hotspotKey)
"@
    $hotspotContent | Out-File -FilePath $hotspotFile -Encoding utf8
}

Write-Host "Created markdown files for each security hotspot in respective severity and category folders under $outputDir\hotspots"
Write-Host "Total: $($issues.Count) open issues and $($hotspots.Count) unreviewed security hotspots processed (remediated/closed items excluded)."
