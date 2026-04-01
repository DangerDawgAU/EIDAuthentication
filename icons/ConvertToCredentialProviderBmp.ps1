<#
.SYNOPSIS
    Converts a standard BMP to Credential Provider BMP format

.DESCRIPTION
    Converts a BMP image to the specific format required by Windows Credential Providers:
    - 32-bit BGRA pixel format
    - BI_BITFIELDS compression (3)
    - Color masks for RGBA channels
    - Proper header structure

.PARAMETER SourcePath
    Path to the source BMP file

.PARAMETER OutputPath
    Path for the output BMP file (default: same as source with _cp suffix)

.EXAMPLE
    .\ConvertToCredentialProviderBmp.ps1 -SourcePath "C:\Users\user\Desktop\credentialProvider.BMP"

.EXAMPLE
    .\ConvertToCredentialProviderBmp.ps1 -SourcePath "source.bmp" -OutputPath "output.bmp"
#>

param(
    [Parameter(Mandatory=$true)]
    [string]$SourcePath,

    [string]$OutputPath
)

$ErrorActionPreference = "Stop"

# Resolve full paths
$SourcePath = (Resolve-Path $SourcePath).Path

if ([string]::IsNullOrEmpty($OutputPath)) {
    $baseName = [System.IO.Path]::GetFileNameWithoutExtension($SourcePath)
    $OutputPath = Join-Path (Split-Path $SourcePath) "${baseName}_cp.bmp"
}

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "Credential Provider BMP Converter" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "Source: $SourcePath" -ForegroundColor White
Write-Host "Output: $OutputPath" -ForegroundColor White
Write-Host ""

# Check if source file exists
if (-not (Test-Path $SourcePath)) {
    Write-Host "ERROR: Source file not found: $SourcePath" -ForegroundColor Red
    exit 1
}

# Create Python script for the conversion
$pythonScript = @'
import sys
from PIL import Image
import struct

def create_cp_bmp(source_path, output_path):
    """Convert BMP to Credential Provider format"""

    # Open source image and convert to RGBA
    img = Image.open(source_path)

    # Resize to 512x512 if needed (standard CP size)
    if img.size != (512, 512):
        print(f"  Resizing from {img.size} to (512, 512)")
        img = img.resize((512, 512), Image.Resampling.LANCZOS)

    # Ensure RGBA format
    if img.mode != 'RGBA':
        print(f"  Converting from {img.mode} to RGBA")
        img = img.convert('RGBA')

    # Get pixel data
    pixels = img.load()
    width, height = img.size

    # BMP file header (14 bytes)
    bfType = 0x4D42  # 'BM'
    bfOffBits = 66    # Pixel data starts after 54 + 12 (color masks)
    pixel_data_size = width * height * 4
    bfSize = 54 + 12 + pixel_data_size  # File size

    # DIB header (BITMAPINFOHEADER - 40 bytes)
    biSize = 40
    biWidth = width
    biHeight = height
    biPlanes = 1
    biBitCount = 32
    biCompression = 3  # BI_BITFIELDS
    biSizeImage = pixel_data_size
    biXPelsPerMeter = 2914  # ~72 DPI
    biYPelsPerMeter = 2914
    biClrUsed = 0
    biClrImportant = 0

    # Color masks (12 bytes) - BGRA format
    red_mask = 0x00FF0000
    green_mask = 0x0000FF00
    blue_mask = 0x000000FF
    alpha_mask = 0xFF000000

    with open(output_path, 'wb') as f:
        # File header
        f.write(struct.pack('<H', bfType))           # 0x00: BFType (2 bytes)
        f.write(struct.pack('<I', bfSize))           # 0x02: BFSize (4 bytes)
        f.write(struct.pack('<H', 0))                # 0x06: BFReserved1 (2 bytes)
        f.write(struct.pack('<H', 0))                # 0x08: BFReserved2 (2 bytes)
        f.write(struct.pack('<I', bfOffBits))        # 0x0A: BFOffBits (4 bytes)

        # DIB header
        f.write(struct.pack('<I', biSize))           # 0x0E: BiSize (4 bytes)
        f.write(struct.pack('<i', biWidth))          # 0x12: BiWidth (4 bytes)
        f.write(struct.pack('<i', biHeight))         # 0x16: BiHeight (4 bytes)
        f.write(struct.pack('<H', biPlanes))         # 0x1A: BiPlanes (2 bytes)
        f.write(struct.pack('<H', biBitCount))       # 0x1C: BiBitCount (2 bytes)
        f.write(struct.pack('<I', biCompression))    # 0x1E: BiCompression (4 bytes)
        f.write(struct.pack('<I', biSizeImage))      # 0x22: BiSizeImage (4 bytes)
        f.write(struct.pack('<i', biXPelsPerMeter))  # 0x26: BiXPelsPerMeter (4 bytes)
        f.write(struct.pack('<i', biYPelsPerMeter))  # 0x2A: BiYPelsPerMeter (4 bytes)
        f.write(struct.pack('<I', biClrUsed))        # 0x2E: BiClrUsed (4 bytes)
        f.write(struct.pack('<I', biClrImportant))  # 0x32: BiClrImportant (4 bytes)

        # Color masks
        f.write(struct.pack('<I', red_mask))         # 0x36: Red mask
        f.write(struct.pack('<I', green_mask))       # 0x3A: Green mask
        f.write(struct.pack('<I', blue_mask))        # 0x3E: Blue mask
        f.write(struct.pack('<I', alpha_mask))       # 0x42: Alpha mask

        # Pixel data (bottom-up, BGRA format)
        for y in range(height - 1, -1, -1):
            for x in range(width):
                r, g, b, a = pixels[x, y]
                # Write as BGRA
                f.write(struct.pack('BBBB', b, g, r, a))

    print(f"  Created: {output_path}")
    return True

if __name__ == '__main__':
    if len(sys.argv) < 3:
        sys.exit(1)
    create_cp_bmp(sys.argv[1], sys.argv[2])
'@

# Save Python script to temp file
$tempPython = [System.IO.Path]::GetTempFileName() + ".py"
$pythonScript | Out-File -FilePath $tempPython -Encoding UTF8

Write-Host "Converting..." -ForegroundColor Yellow

try {
    # Run Python script
    $pythonCmd = Get-Command python -ErrorAction SilentlyContinue
    if ($pythonCmd) {
        $result = & python $tempPython $SourcePath $OutputPath 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Host ""
            Write-Host "============================================================" -ForegroundColor Green
            Write-Host "Conversion Successful!" -ForegroundColor Green
            Write-Host "============================================================" -ForegroundColor Green
            Write-Host "Output: $OutputPath" -ForegroundColor White

            # Show file info
            $fileInfo = Get-Item $OutputPath
            Write-Host "Size: $($fileInfo.Length) bytes" -ForegroundColor White
            Write-Host ""
        } else {
            Write-Host "ERROR: Python script failed" -ForegroundColor Red
            Write-Host $result
            exit 1
        }
    } else {
        Write-Host "ERROR: Python not found. Please install Python 3.x with Pillow:" -ForegroundColor Red
        Write-Host "  pip install Pillow" -ForegroundColor Yellow
        exit 1
    }
} finally {
    # Clean up temp file
    Remove-Item $tempPython -ErrorAction SilentlyContinue
}
