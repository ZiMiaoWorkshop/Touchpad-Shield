param(
    [string]$SourcePng = (Join-Path (Split-Path -Parent $PSScriptRoot) "Picture\Touchpad Shield LOGO.png"),
    [string]$AppIco = (Join-Path (Split-Path -Parent $PSScriptRoot) "src\TouchpadShield\Assets\TouchpadShield.ico"),
    [string]$InstallerIco = (Join-Path (Split-Path -Parent $PSScriptRoot) "installer\TouchpadShield.ico")
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $SourcePng)) {
    throw "Logo PNG not found: $SourcePng"
}

$assetsDir = Split-Path $AppIco -Parent
if (-not (Test-Path $assetsDir)) {
    New-Item -ItemType Directory -Force -Path $assetsDir | Out-Null
}
Copy-Item -Force $SourcePng (Join-Path $assetsDir "TouchpadShieldLogo.png")

Add-Type -ReferencedAssemblies System.Drawing @"
using System;
using System.IO;

public static class TouchpadShieldIconWriter
{
    public static void SaveMultiSizeIcon(string pngPath, string icoPath)
    {
        int[] sizes = new[] { 16, 24, 32, 48, 64, 128, 256 };
        using (var source = new System.Drawing.Bitmap(pngPath))
        using (var memory = new MemoryStream())
        using (var writer = new BinaryWriter(memory))
        {
            writer.Write((ushort)0);
            writer.Write((ushort)1);
            writer.Write((ushort)sizes.Length);

            var imageData = new byte[sizes.Length][];
            for (int i = 0; i < sizes.Length; i++)
            {
                imageData[i] = CreatePngImageData(source, sizes[i]);
            }

            int offset = 6 + (16 * sizes.Length);
            for (int i = 0; i < sizes.Length; i++)
            {
                int size = sizes[i];
                writer.Write((byte)(size >= 256 ? 0 : size));
                writer.Write((byte)(size >= 256 ? 0 : size));
                writer.Write((byte)0);
                writer.Write((byte)0);
                writer.Write((ushort)1);
                writer.Write((ushort)32);
                writer.Write((uint)imageData[i].Length);
                writer.Write((uint)offset);
                offset += imageData[i].Length;
            }

            for (int i = 0; i < sizes.Length; i++)
            {
                writer.Write(imageData[i]);
            }

            File.WriteAllBytes(icoPath, memory.ToArray());
        }
    }

    private static byte[] CreatePngImageData(System.Drawing.Bitmap source, int size)
    {
        using (var bitmap = new System.Drawing.Bitmap(size, size, System.Drawing.Imaging.PixelFormat.Format32bppArgb))
        {
            using (var graphics = System.Drawing.Graphics.FromImage(bitmap))
            {
                graphics.CompositingQuality = System.Drawing.Drawing2D.CompositingQuality.HighQuality;
                graphics.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.HighQualityBicubic;
                graphics.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.HighQuality;
                graphics.PixelOffsetMode = System.Drawing.Drawing2D.PixelOffsetMode.HighQuality;
                graphics.Clear(System.Drawing.Color.Transparent);
                graphics.DrawImage(source, 0, 0, size, size);
            }

            using (var stream = new MemoryStream())
            {
                bitmap.Save(stream, System.Drawing.Imaging.ImageFormat.Png);
                return stream.ToArray();
            }
        }
    }
}
"@

[TouchpadShieldIconWriter]::SaveMultiSizeIcon($SourcePng, $AppIco)
Copy-Item -Force $AppIco $InstallerIco
Write-Host "Generated icons:"
Write-Host "  $AppIco"
Write-Host "  $InstallerIco"
