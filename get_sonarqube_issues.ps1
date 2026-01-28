# This script fetches all issues from a public SonarCloud project and saves them to a JSON file.

# SonarCloud project key
$projectKey = "DangerDawgAU_EIDAuthentication"
$sonarCloudUrl = "https://sonarcloud.io"

# Create a directory to store the issues
$outputDir = "sonarqube_issues"
if (-not (Test-Path -Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir
}

$page = 1
$pageSize = 500
$issues = @()

do {
    # Construct the API URL
    $apiUrl = "$sonarCloudUrl/api/issues/search?componentKeys=$projectKey&p=$page&ps=$pageSize"

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

# Create individual markdown files for each issue, organized by severity
foreach ($issue in $issues) {
    $severity = $issue.severity
    $severityDir = Join-Path -Path $outputDir -ChildPath $severity
    if (-not (Test-Path -Path $severityDir)) {
        New-Item -ItemType Directory -Path $severityDir
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

Write-Host "Created markdown files for each issue in respective severity folders under $outputDir"
