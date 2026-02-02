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

    # Remove EID credential mappings from LSA Private Data
    # These are stored as L$_EID_<8-digit-hex-RID> for each local user
    Write-Host "Removing EID credential mappings from LSA storage..." -ForegroundColor Yellow

    # P/Invoke definitions for LSA API
    Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;

public class LsaHelper
{
    [StructLayout(LayoutKind.Sequential)]
    public struct LSA_OBJECT_ATTRIBUTES
    {
        public uint Length;
        public IntPtr RootDirectory;
        public IntPtr ObjectName;
        public uint Attributes;
        public IntPtr SecurityDescriptor;
        public IntPtr SecurityQualityOfService;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct LSA_UNICODE_STRING
    {
        public ushort Length;
        public ushort MaximumLength;
        public IntPtr Buffer;
    }

    [DllImport("advapi32.dll", SetLastError = true)]
    public static extern uint LsaOpenPolicy(
        IntPtr SystemName,
        ref LSA_OBJECT_ATTRIBUTES ObjectAttributes,
        uint DesiredAccess,
        out IntPtr PolicyHandle);

    [DllImport("advapi32.dll", SetLastError = true)]
    public static extern uint LsaStorePrivateData(
        IntPtr PolicyHandle,
        ref LSA_UNICODE_STRING KeyName,
        IntPtr PrivateData);

    [DllImport("advapi32.dll", SetLastError = true)]
    public static extern uint LsaRetrievePrivateData(
        IntPtr PolicyHandle,
        ref LSA_UNICODE_STRING KeyName,
        out IntPtr PrivateData);

    [DllImport("advapi32.dll")]
    public static extern uint LsaClose(IntPtr ObjectHandle);

    [DllImport("advapi32.dll")]
    public static extern uint LsaFreeMemory(IntPtr Buffer);

    [DllImport("advapi32.dll")]
    public static extern int LsaNtStatusToWinError(uint Status);

    public const uint POLICY_CREATE_SECRET = 0x00000020;
    public const uint READ_CONTROL = 0x00020000;
    public const uint WRITE_OWNER = 0x00080000;
    public const uint WRITE_DAC = 0x00040000;

    public static LSA_UNICODE_STRING CreateLsaString(string str)
    {
        var lsaStr = new LSA_UNICODE_STRING();
        lsaStr.Buffer = Marshal.StringToHGlobalUni(str);
        lsaStr.Length = (ushort)(str.Length * 2);
        lsaStr.MaximumLength = lsaStr.Length;
        return lsaStr;
    }
}
"@

    $lsaHandle = [IntPtr]::Zero
    $objAttr = New-Object LsaHelper+LSA_OBJECT_ATTRIBUTES
    $access = [LsaHelper]::POLICY_CREATE_SECRET -bor [LsaHelper]::READ_CONTROL -bor [LsaHelper]::WRITE_OWNER -bor [LsaHelper]::WRITE_DAC
    $result = [LsaHelper]::LsaOpenPolicy([IntPtr]::Zero, [ref]$objAttr, $access, [ref]$lsaHandle)

    if ($result -eq 0) {
        $localUsers = Get-LocalUser
        $mappingsRemoved = 0
        foreach ($user in $localUsers) {
            try {
                $sid = $user.SID
                # Extract RID (last sub-authority) from the SID
                $sidParts = $sid.Value -split '-'
                $rid = [uint32]$sidParts[$sidParts.Length - 1]
                $keyName = "L`$_EID_{0:X8}" -f $rid

                # Check if this LSA key exists by attempting to retrieve it
                $lsaKeyName = [LsaHelper]::CreateLsaString($keyName)
                $privateData = [IntPtr]::Zero
                $retrieveResult = [LsaHelper]::LsaRetrievePrivateData($lsaHandle, [ref]$lsaKeyName, [ref]$privateData)

                if ($retrieveResult -eq 0) {
                    if ($privateData -ne [IntPtr]::Zero) {
                        [LsaHelper]::LsaFreeMemory($privateData) | Out-Null
                    }
                    # Key exists - delete it by storing null
                    $deleteResult = [LsaHelper]::LsaStorePrivateData($lsaHandle, [ref]$lsaKeyName, [IntPtr]::Zero)
                    if ($deleteResult -eq 0) {
                        Write-Host "  Removed credential mapping for user: $($user.Name)" -ForegroundColor Green
                        $mappingsRemoved++
                        $removed++
                    } else {
                        $winErr = [LsaHelper]::LsaNtStatusToWinError($deleteResult)
                        Write-Host "  Error removing mapping for $($user.Name): Win32 error $winErr" -ForegroundColor Red
                        $errors++
                    }
                }
                [System.Runtime.InteropServices.Marshal]::FreeHGlobal($lsaKeyName.Buffer)
            } catch {
                Write-Host "  Error processing user $($user.Name): $_" -ForegroundColor Red
                $errors++
            }
        }
        [LsaHelper]::LsaClose($lsaHandle) | Out-Null

        if ($mappingsRemoved -eq 0) {
            Write-Host "  No EID credential mappings found." -ForegroundColor Gray
        }
    } else {
        $winErr = [LsaHelper]::LsaNtStatusToWinError($result)
        Write-Host "  Failed to open LSA policy (Win32 error $winErr). Run as Administrator." -ForegroundColor Red
        $errors++
    }

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
