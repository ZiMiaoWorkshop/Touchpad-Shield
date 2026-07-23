param(
    [string]$SourceImage = (Join-Path (Split-Path -Parent $PSScriptRoot) "Picture\ZiMiaoWorkshop LOGO.jpg"),
    [string]$WelcomeFinishBmp = (Join-Path (Split-Path -Parent $PSScriptRoot) "installer\welcome-finish.bmp")
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $SourceImage)) {
    throw "Logo image not found: $SourceImage"
}

Add-Type -AssemblyName System.Drawing

function Save-NsisWelcomeFinishBitmap {
    param(
        [string]$ImagePath,
        [string]$BmpPath,
        [int]$Width = 164,
        [int]$Height = 314
    )

    $bitmap = New-Object System.Drawing.Bitmap $Width, $Height, ([System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
        $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
        $graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::ClearTypeGridFit

        $backgroundRect = New-Object System.Drawing.Rectangle 0, 0, $Width, $Height
        $backgroundBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
            $backgroundRect,
            [System.Drawing.Color]::FromArgb(255, 24, 28, 34),
            [System.Drawing.Color]::FromArgb(255, 14, 18, 24),
            [System.Drawing.Drawing2D.LinearGradientMode]::Vertical)
        $graphics.FillRectangle($backgroundBrush, $backgroundRect)
        $backgroundBrush.Dispose()

        $logo = [System.Drawing.Image]::FromFile($ImagePath)
        try {
            $logoSize = 112
            $logoX = [int](($Width - $logoSize) / 2)
            $logoY = 56
            $graphics.DrawImage($logo, $logoX, $logoY, $logoSize, $logoSize)
        }
        finally {
            $logo.Dispose()
        }

        $titleFont = New-Object System.Drawing.Font -ArgumentList @("Segoe UI", 11, [System.Drawing.FontStyle]::Bold)
        $subtitleFont = New-Object System.Drawing.Font -ArgumentList @("Segoe UI", 9, [System.Drawing.FontStyle]::Regular)
        $titleBrush = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(255, 235, 245, 248))
        $subtitleBrush = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(255, 120, 190, 205))

        $title = "ZiMiaoWorkshop"
        $subtitle = "Touchpad Shield"
        $titleSize = $graphics.MeasureString($title, $titleFont)
        $subtitleSize = $graphics.MeasureString($subtitle, $subtitleFont)
        $titleX = [int](($Width - $titleSize.Width) / 2)
        $subtitleX = [int](($Width - $subtitleSize.Width) / 2)
        $textTop = $logoY + $logoSize + 18

        $graphics.DrawString($title, $titleFont, $titleBrush, $titleX, $textTop)
        $graphics.DrawString($subtitle, $subtitleFont, $subtitleBrush, $subtitleX, $textTop + 24)

        $lineY = $textTop + 52
        $lineWidth = 72
        $lineX = [int](($Width - $lineWidth) / 2)
        $linePen = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(255, 0, 151, 167)), 2
        $graphics.DrawLine($linePen, $lineX, $lineY, $lineX + $lineWidth, $lineY)
        $linePen.Dispose()

        $titleFont.Dispose()
        $subtitleFont.Dispose()
        $titleBrush.Dispose()
        $subtitleBrush.Dispose()
    }
    finally {
        $graphics.Dispose()
    }

    $bitmap.Save($BmpPath, [System.Drawing.Imaging.ImageFormat]::Bmp)
    $bitmap.Dispose()
}

Save-NsisWelcomeFinishBitmap -ImagePath $SourceImage -BmpPath $WelcomeFinishBmp
Write-Host "Generated installer bitmap:"
Write-Host "  $WelcomeFinishBmp"
