# Install Virtual Smart Card Reader and create a virtual smart card
# Requires TPM 1.2 or higher
# Run as Administrator

Write-Host "Checking TPM availability..."
$tpm = Get-Tpm -ErrorAction SilentlyContinue

if ($null -eq $tpm) {
    Write-Host "ERROR: No TPM detected on this system." -ForegroundColor Red
    Write-Host "Virtual smart cards require TPM 1.2 or higher." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Alternatives:" -ForegroundColor Cyan
    Write-Host "1. Enable TPM in VM settings (if running in VM)"
    Write-Host "2. Use a physical smart card reader"
    Write-Host "3. Install a third-party virtual smart card reader driver"
    exit 1
}

if (-not $tpm.TpmPresent) {
    Write-Host "ERROR: TPM is not present." -ForegroundColor Red
    exit 1
}

if (-not $tpm.TpmReady) {
    Write-Host "WARNING: TPM is not ready. Attempting to initialize..." -ForegroundColor Yellow
    Initialize-Tpm
}

Write-Host "TPM is available and ready." -ForegroundColor Green
Write-Host ""

# Create a virtual smart card
Write-Host "Creating virtual smart card..."
$pin = "12345678"  # Default PIN for testing
$adminKey = "010203040506070801020304050607080102030405060708"  # Default admin key

try {
    # Create virtual smart card
    $result = TpmVscMgr.exe create /name "EID Test Card" /pin $pin /adminkey $adminKey /generate

    if ($LASTEXITCODE -eq 0) {
        Write-Host "Virtual smart card created successfully!" -ForegroundColor Green
        Write-Host "PIN: $pin" -ForegroundColor Cyan
        Write-Host ""

        # Start Smart Card services
        Write-Host "Starting Smart Card services..."
        Start-Service -Name SCardSvr
        Start-Service -Name ScDeviceEnum

        Write-Host ""
        Write-Host "Service Status:"
        Get-Service -Name SCardSvr, ScDeviceEnum | Format-Table Name, Status, StartType

        Write-Host ""
        Write-Host "Virtual smart card is ready for use." -ForegroundColor Green
    } else {
        Write-Host "Failed to create virtual smart card." -ForegroundColor Red
        Write-Host "Exit code: $LASTEXITCODE" -ForegroundColor Yellow
    }
} catch {
    Write-Host "Error creating virtual smart card: $_" -ForegroundColor Red
}
