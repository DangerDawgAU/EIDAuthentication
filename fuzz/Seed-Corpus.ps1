<#
.SYNOPSIS
    Write well-formed seed inputs for each fuzz target.

.DESCRIPTION
    libFuzzer explores far faster from valid structure than from random bytes:
    every one of these buffers passes its validator, so mutations start at the
    boundary that matters instead of being rejected by the first length check.

    Also writes the known-bad proof-of-concept inputs. Those must never crash
    the target - they exist so a regression shows up as a corpus entry that
    suddenly fails rather than as a silent gap.

    Safe to re-run; files are overwritten.
#>

$ErrorActionPreference = "Stop"
$corpusRoot = Join-Path $PSScriptRoot "corpus"

function Write-Seed {
    param([string]$Target, [string]$Name, [byte[]]$Bytes)
    $dir = Join-Path $corpusRoot $Target
    New-Item -ItemType Directory -Force $dir | Out-Null
    [IO.File]::WriteAllBytes((Join-Path $dir "$Name.bin"), $Bytes)
}

# Little-endian helpers matching the on-wire structs.
function U32 { param([uint32]$v) [BitConverter]::GetBytes($v) }
function U16 { param([uint16]$v) [BitConverter]::GetBytes($v) }

# ---------------------------------------------------------------------------
# tokenmessage - EID_CHALLENGE_MESSAGE / EID_RESPONSE_MESSAGE
# Layout: Signature[8], MessageType, Flags, UsernameLen, UsernameOffset,
#         ChallengeLen, ChallengeOffset, Version   (all DWORD after Signature)
# ---------------------------------------------------------------------------
$sig = [Text.Encoding]::ASCII.GetBytes("EIDAuth")  + @(0x00)   # 8 bytes
$hdr = 8 + 7 * 4                                              # 36 bytes

$challenge = @()
$challenge += $sig
$challenge += U32 2            # MessageType = EIDMTChallenge
$challenge += U32 0            # Flags
$challenge += U32 16           # UsernameLen  (even, as WCHARs)
$challenge += U32 ($hdr + 16)  # UsernameOffset
$challenge += U32 16           # ChallengeLen
$challenge += U32 $hdr         # ChallengeOffset
$challenge += U32 1            # Version
$challenge += (1..32 | ForEach-Object { [byte]0x41 })
Write-Seed -Target tokenmessage -Name "valid-challenge" -Bytes ([byte[]]$challenge)

# Response: Signature[8], MessageType, ResponseLen, ResponseOffset, Version
$rhdr = 8 + 4 * 4
$response = @()
$response += $sig
$response += U32 3             # MessageType = EIDMTResponse
$response += U32 24            # ResponseLen
$response += U32 $rhdr         # ResponseOffset
$response += U32 1             # Version
$response += (1..24 | ForEach-Object { [byte]0x42 })
Write-Seed -Target tokenmessage -Name "valid-response" -Bytes ([byte[]]$response)

# ---------------------------------------------------------------------------
# cspinfo - EID_SMARTCARD_CSP_INFO (packed 1)
# dwCspInfoLen, MessageType, union{ULONG64}, flags, KeySpec,
# nCardName, nReaderName, nContainerName, nCSPName, bBuffer[4 TCHAR]
# ---------------------------------------------------------------------------
$cspHeader = 4 + 4 + 8 + 4 + 4 + 4 + 4 + 4 + 4     # 40 bytes, pack(1)
$nameChars = 24
$cspTotal  = $cspHeader + $nameChars * 2

$csp = @()
$csp += U32 $cspTotal   # dwCspInfoLen == real size
$csp += U32 1           # MessageType
$csp += [byte[]]::new(8)
$csp += U32 0           # flags
$csp += U32 1           # KeySpec (AT_KEYEXCHANGE)
$csp += U32 0           # nCardNameOffset      (0 = absent)
$csp += U32 6           # nReaderNameOffset    (WCHAR units)
$csp += U32 12          # nContainerNameOffset
$csp += U32 1           # nCSPNameOffset
$names = [byte[]]::new($nameChars * 2)
function Put-Wide { param([byte[]]$Buf, [int]$CharIndex, [string]$Text)
    $b = [Text.Encoding]::Unicode.GetBytes($Text + "`0")
    [Array]::Copy($b, 0, $Buf, $CharIndex * 2, $b.Length)
}
Put-Wide $names 1  "MyEID"       # nCSPNameOffset = 1
Put-Wide $names 6  "Reader 0"    # nReaderNameOffset = 6
Put-Wide $names 12 "Container"   # nContainerNameOffset = 12
$csp += $names
Write-Seed -Target cspinfo -Name "valid-cspinfo" -Bytes ([byte[]]$csp)

# Known-bad: 40-byte buffer claiming a 64 KB interior (the core defect).
$bad = @()
$bad += U32 0x10000
$bad += U32 1
$bad += [byte[]]::new(8)
$bad += U32 0
$bad += U32 1
$bad += U32 0xFFF0
$bad += U32 0
$bad += U32 0
$bad += U32 0
Write-Seed -Target cspinfo -Name "poc-inner-length-exceeds" -Bytes ([byte[]]$bad)

# ---------------------------------------------------------------------------
# privatedata - EID_PRIVATE_DATA
# dwType(4), 6x USHORT(12), Hash[32], Data[...]
# ---------------------------------------------------------------------------
$pdHeader = 4 + 12 + 32       # 48 bytes before Data
$pd = @()
$pd += U32 1                  # dwType = eidpdtCrypted
$pd += U16 0                  # dwCertificatOffset
$pd += U16 16                 # dwCertificatSize
$pd += U16 16                 # dwSymetricKeyOffset
$pd += U16 16                 # dwSymetricKeySize
$pd += U16 32                 # dwPasswordOffset
$pd += U16 16                 # usPasswordLen (non-zero, one AES block)
$pd += [byte[]]::new(32)      # Hash
$pd += (1..48 | ForEach-Object { [byte]0x43 })
Write-Seed -Target privatedata -Name "valid-blob" -Bytes ([byte[]]$pd)

# Known-bad: usPasswordLen = 0 (the dwRoundNumber underflow).
$pdBad = @()
$pdBad += U32 1
$pdBad += U16 0
$pdBad += U16 16
$pdBad += U16 16
$pdBad += U16 16
$pdBad += U16 32
$pdBad += U16 0
$pdBad += [byte[]]::new(32)
$pdBad += (1..48 | ForEach-Object { [byte]0x43 })
Write-Seed -Target privatedata -Name "poc-zero-password-len" -Bytes ([byte[]]$pdBad)

# ---------------------------------------------------------------------------
# json - shapes drawn from the real .eidm schema plus the escape edge cases
# ---------------------------------------------------------------------------
$jsonSeeds = @{
    "schema-like"   = '{"version":1,"credentials":[{"rid":1001,"cert":"4d5a90","flags":0}],"groups":["Users"]}'
    "escapes"       = '{"a":"A\t\n\\\"\/","b":-12345,"c":true,"d":null}'
    "nested"        = '{"a":{"b":{"c":[[1,2],[3,4]]}}}'
    "truncated-esc" = '{"a":"\u00'
    "empty-object"  = '{}'
}
foreach ($kv in $jsonSeeds.GetEnumerator()) {
    Write-Seed -Target json -Name $kv.Key -Bytes ([Text.Encoding]::UTF8.GetBytes($kv.Value))
}

Write-Host "Seed corpora written under $corpusRoot" -ForegroundColor Green
Get-ChildItem $corpusRoot -Directory | ForEach-Object {
    $n = (Get-ChildItem $_.FullName -File).Count
    Write-Host ("  {0,-14} {1} file(s)" -f $_.Name, $n) -ForegroundColor Gray
}
