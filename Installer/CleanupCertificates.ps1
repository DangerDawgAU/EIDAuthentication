# Cleanup certificates created by EID Authentication
# Run as Administrator

Write-Host "Removing certificates created by EID Authentication..." -ForegroundColor Cyan

$removed = 0
$errors = 0

try {
    # Remove from LocalMachine\Root (self-signed root certificates)
    Write-Host "Checking LocalMachine\Root store..." -ForegroundColor Yellow
    $rootStore = New-Object System.Security.Cryptography.X509Certificates.X509Store("Root", "LocalMachine")
    $rootStore.Open([System.Security.Cryptography.X509Certificates.OpenFlags]::ReadWrite)

    $certsToRemove = $rootStore.Certificates | Where-Object {
        $_.Subject -like "*EID Authentication*" -or
        $_.Issuer -like "*EID Authentication*" -or
        $_.Subject -like "*My Smart Logon*" -or
        $_.Issuer -like "*My Smart Logon*"
    }

    foreach ($cert in $certsToRemove) {
        try {
            $rootStore.Remove($cert)
            Write-Host "  Removed: $($cert.Subject)" -ForegroundColor Green
            $removed++
        } catch {
            Write-Host "  Error removing: $($cert.Subject) - $_" -ForegroundColor Red
            $errors++
        }
    }

    $rootStore.Close()

    # Remove from LocalMachine\My (user certificates)
    Write-Host "Checking LocalMachine\My store..." -ForegroundColor Yellow
    $myStore = New-Object System.Security.Cryptography.X509Certificates.X509Store("My", "LocalMachine")
    $myStore.Open([System.Security.Cryptography.X509Certificates.OpenFlags]::ReadWrite)

    $certsToRemove = $myStore.Certificates | Where-Object {
        $_.Subject -like "*EID Authentication*" -or
        $_.Issuer -like "*EID Authentication*" -or
        $_.Subject -like "*My Smart Logon*" -or
        $_.Issuer -like "*My Smart Logon*" -or
        $_.EnhancedKeyUsageList.FriendlyName -contains "Smart Card Logon"
    }

    foreach ($cert in $certsToRemove) {
        try {
            $myStore.Remove($cert)
            Write-Host "  Removed: $($cert.Subject)" -ForegroundColor Green
            $removed++
        } catch {
            Write-Host "  Error removing: $($cert.Subject) - $_" -ForegroundColor Red
            $errors++
        }
    }

    $myStore.Close()

    # Remove from LocalMachine\TrustedPeople
    Write-Host "Checking LocalMachine\TrustedPeople store..." -ForegroundColor Yellow
    $trustedStore = New-Object System.Security.Cryptography.X509Certificates.X509Store("TrustedPeople", "LocalMachine")
    $trustedStore.Open([System.Security.Cryptography.X509Certificates.OpenFlags]::ReadWrite)

    $certsToRemove = $trustedStore.Certificates | Where-Object {
        $_.Subject -like "*EID Authentication*" -or
        $_.Issuer -like "*EID Authentication*" -or
        $_.Subject -like "*My Smart Logon*" -or
        $_.Issuer -like "*My Smart Logon*"
    }

    foreach ($cert in $certsToRemove) {
        try {
            $trustedStore.Remove($cert)
            Write-Host "  Removed: $($cert.Subject)" -ForegroundColor Green
            $removed++
        } catch {
            Write-Host "  Error removing: $($cert.Subject) - $_" -ForegroundColor Red
            $errors++
        }
    }

    $trustedStore.Close()

    Write-Host ""
    Write-Host "Certificate cleanup complete!" -ForegroundColor Green
    Write-Host "Certificates removed: $removed" -ForegroundColor White
    if ($errors -gt 0) {
        Write-Host "Errors: $errors" -ForegroundColor Red
    }

} catch {
    Write-Host "Error during certificate cleanup: $_" -ForegroundColor Red
    exit 1
}

exit 0
