# Convert the supplied Home background into the native presentation dimensions.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$source = [Drawing.Image]::FromFile((Join-Path $PSScriptRoot 'home-background.png'))
try {
    $bitmap = [Drawing.Bitmap]::new(3840, 2160, [Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $graphics = [Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $graphics.DrawImage($source, [Drawing.Rectangle]::new(0, 0, 3840, 2160))
        foreach ($name in @('pic0.png', 'pic1.png')) {
            $bitmap.Save((Join-Path $PSScriptRoot $name), [Drawing.Imaging.ImageFormat]::Png)
        }
    } finally { $graphics.Dispose(); $bitmap.Dispose() }
    # The shell supplies the title text; keep its optional foreground layer transparent.
    $foreground = [Drawing.Bitmap]::new(3840, 2160, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try { $foreground.Save((Join-Path $PSScriptRoot 'pic2.png'), [Drawing.Imaging.ImageFormat]::Png) }
    finally { $foreground.Dispose() }
} finally { $source.Dispose() }
