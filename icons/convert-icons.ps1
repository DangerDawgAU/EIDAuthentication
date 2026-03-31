Add-Type -AssemblyName System.Drawing

function Convert-PngToIco {
    param($pngPath, $icoPath)

    $png = [System.Drawing.Image]::FromFile($pngPath)

    # Create ICO file
    $fs = [System.IO.FileStream]::new($icoPath, [System.IO.FileMode]::Create)
    $bw = [System.IO.BinaryWriter]::new($fs)

    # ICO header: reserved(2), type(2)=1, count(2)
    $bw.Write([UInt16]0)
    $bw.Write([UInt16]1)
    $bw.Write([UInt16]1)

    # Get PNG data
    $ms = [System.IO.MemoryStream]::new()
    $png.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $pngBytes = $ms.ToArray()
    $ms.Close()

    # Directory entry
    $bw.Write([Byte]0)      # 0 width = 256
    $bw.Write([Byte]0)      # 0 height = 256
    $bw.Write([Byte]0)      # 0 colors
    $bw.Write([Byte]0)      # reserved
    $bw.Write([UInt16]0)    # color planes
    $bw.Write([UInt16]32)   # 32 bpp
    $bw.Write([UInt32]$pngBytes.Length)
    $bw.Write([UInt32]22)

    # Write PNG data
    $bw.Write($pngBytes)

    $bw.Close()
    $fs.Close()
    $png.Dispose()

    Write-Host "Converted: $pngPath -> $icoPath"
}

function Convert-PngToBmp {
    param($pngPath, $bmpPath, $width, $height)

    $png = [System.Drawing.Image]::FromFile($pngPath)

    # Create new bitmap with specified size
    $bmp = [System.Drawing.Bitmap]::new($width, $height)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.DrawImage($png, 0, 0, $width, $height)
    $g.Dispose()

    # Save as BMP
    $bmp.Save($bmpPath, [System.Drawing.Imaging.ImageFormat]::Bmp)
    $bmp.Dispose()
    $png.Dispose()

    Write-Host "Converted: $pngPath -> $bmpPath ($width x $height)"
}

# Convert based on user mapping
Convert-PngToIco 'bulk-image-crop/Gemini_Generated_Image_tgdtf3tgdtf3tgdt.png' 'app_configuration_wizard.ico'
Convert-PngToIco 'bulk-image-crop/Gemini_Generated_Image_tgdtf3tgdtf3tgdt(1).png' 'app_log_manager.ico'
Convert-PngToIco 'bulk-image-crop/Gemini_Generated_Image_tgdtf3tgdtf3tgdt(2).png' 'app_migrate_cli.ico'
Convert-PngToIco 'bulk-image-crop/Gemini_Generated_Image_tgdtf3tgdtf3tgdt(3).png' 'app_migrate_gui.ico'
Convert-PngToIco 'bulk-image-crop/Gemini_Generated_Image_tgdtf3tgdtf3tgdt(5).png' 'app_installer.ico'

# Convert credential provider tile (48x48 BMP)
Convert-PngToBmp 'bulk-image-crop/Gemini_Generated_Image_tgdtf3tgdtf3tgdt(6).png' 'cred_tile_image.bmp' 48 48

Write-Host "Done!"
