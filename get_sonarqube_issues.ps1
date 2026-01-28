# This script fetches all issues from a public SonarCloud project and saves them to a JSON file.

# SonarCloud project key
$projectKey = "DangerDawgAU_EIDAuthentication"
$sonarCloudUrl = "https://sonarcloud.io"

# Create a directory to store the issues
$outputDir = "sonarqube"
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

# Save the issues to a JSON file
$outputFile = Join-Path -Path $outputDir -ChildPath "sonarqube_issues.json"
$issues | ConvertTo-Json -Depth 10 | Out-File -FilePath $outputFile -Encoding utf8

Write-Host "Successfully fetched $($issues.Count) issues and saved them to $outputFile"

# Create individual markdown files for each issue
$issuesDir = Join-Path -Path $outputDir -ChildPath "issues"
if (-not (Test-Path -Path $issuesDir)) {
    New-Item -ItemType Directory -Path $issuesDir
}

foreach ($issue in $issues) {
    $issueKey = $issue.key
    $issueFile = Join-Path -Path $issuesDir -ChildPath "$issueKey.md"
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

Write-Host "Created markdown files for each issue in $issuesDir"
