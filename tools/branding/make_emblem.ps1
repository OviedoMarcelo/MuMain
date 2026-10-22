# Writes the MU Cyber emblem of the quest tracker (Data/Interface/MuCyberEmblem.OZT) from the biggest
# image of the game icon, so that both show the same emblem.
#
#   powershell -File tools/branding/make_emblem.ps1 [-Icon <icon.ico>] [-Output <file.OZT>] [-Size 64]
param(
    [string]$Icon = (Join-Path $PSScriptRoot "..\..\src\source\App\Platform\Windows\icon1.ico"),
    [string]$Output = (Join-Path $PSScriptRoot "..\..\src\bin\Data\Interface\MuCyberEmblem.OZT"),
    [int]$Size = 64
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

# The icon keeps its images as a list; the biggest one is a PNG.
function Read-BiggestIconImage([byte[]]$ico) {
    $count = [BitConverter]::ToUInt16($ico, 4)
    $best = $null
    for ($index = 0; $index -lt $count; $index++) {
        $entry = 6 + 16 * $index
        $length = [BitConverter]::ToUInt32($ico, $entry + 8)
        if ($null -eq $best -or $length -gt $best.Length) {
            $best = @{ Offset = [BitConverter]::ToUInt32($ico, $entry + 12); Length = $length }
        }
    }

    $stream = New-Object IO.MemoryStream(, $ico[$best.Offset..($best.Offset + $best.Length - 1)])
    return [Drawing.Image]::FromStream($stream)
}

function Resize-Image([Drawing.Image]$image, [int]$size) {
    $bitmap = New-Object Drawing.Bitmap($size, $size, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [Drawing.Graphics]::FromImage($bitmap)
    $graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $graphics.DrawImage($image, 0, 0, $size, $size)
    $graphics.Dispose()
    return $bitmap
}

# An OZT is a 32-bit uncompressed TGA behind a 4-byte header, with its rows from the bottom up, in BGRA.
function Write-Ozt([Drawing.Bitmap]$bitmap, [string]$path) {
    $width = $bitmap.Width
    $height = $bitmap.Height
    $data = New-Object IO.MemoryStream
    $writer = New-Object IO.BinaryWriter($data)
    $writer.Write([byte[]](0, 0, 2, 0))
    $writer.Write([byte[]](0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0))
    $writer.Write([uint16]$width)
    $writer.Write([uint16]$height)
    $writer.Write([byte[]](32, 8))
    for ($y = $height - 1; $y -ge 0; $y--) {
        for ($x = 0; $x -lt $width; $x++) {
            $pixel = $bitmap.GetPixel($x, $y)
            $writer.Write([byte[]]($pixel.B, $pixel.G, $pixel.R, $pixel.A))
        }
    }

    $writer.Flush()
    [IO.File]::WriteAllBytes($path, $data.ToArray())
}

$image = Read-BiggestIconImage ([IO.File]::ReadAllBytes((Resolve-Path $Icon)))
$emblem = Resize-Image $image $Size
Write-Ozt $emblem ([IO.Path]::GetFullPath($Output))
Write-Host "Emblem written to $Output ($Size x $Size)."
