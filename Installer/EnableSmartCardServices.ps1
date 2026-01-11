# Enable and start Smart Card services
# Run this as Administrator

Write-Host "Configuring Smart Card services..."

# Set services to Manual start (demand start)
Set-Service -Name "SCardSvr" -StartupType Manual
Set-Service -Name "ScDeviceEnum" -StartupType Manual

Write-Host "Starting Smart Card Resource Manager..."
Start-Service -Name "SCardSvr"

Write-Host "Starting Smart Card Device Enumeration Service..."
Start-Service -Name "ScDeviceEnum"

# Verify status
Write-Host "`nService Status:"
Get-Service -Name "SCardSvr" | Format-Table -Property Name, Status, StartType
Get-Service -Name "ScDeviceEnum" | Format-Table -Property Name, Status, StartType

Write-Host "`nSmart Card services are now running."
