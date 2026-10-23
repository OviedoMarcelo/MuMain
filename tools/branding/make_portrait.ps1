# Writes the portrait of an NPC for the dialogs of the quests (Data/Interface/NpcPortraits/<number>.OZJ)
# from a PNG or JPEG. The image is cut to a square (keeping its top, where the face usually is) and resized.
#
#   powershell -File tools/branding/make_portrait.ps1 -Image <portrait.png> -NpcNumber <number> [-Size 256]
#
# The number is the one of the NPC definition, e.g. the one which /npcids shows in the game.
param(
    [Parameter(Mandatory = $true)][string]$Image,
    [Parameter(Mandatory = $true)][int]$NpcNumber,
    # Empty: Data/Interface/NpcPortraits of the client. It's set below, because $PSScriptRoot is
    # empty in the defaults of the parameters in Windows PowerShell 5.1.
    [string]$OutputDirectory = "",
    [int]$Size = 256,
    [int]$Quality = 92
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

# The client skips this many bytes in front of the JPEG.
$OzjHeaderLength = 24

function Get-SquareTop([Drawing.Image]$image) {
    $side = [Math]::Min($image.Width, $image.Height)
    $x = [int](($image.Width - $side) / 2)
    return New-Object Drawing.Rectangle($x, 0, $side, $side)
}

function Resize-Square([Drawing.Image]$image, [int]$size) {
    $bitmap = New-Object Drawing.Bitmap($size, $size, [Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $graphics = [Drawing.Graphics]::FromImage($bitmap)
    $graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $target = New-Object Drawing.Rectangle(0, 0, $size, $size)
    $graphics.DrawImage($image, $target, (Get-SquareTop $image), [Drawing.GraphicsUnit]::Pixel)
    $graphics.Dispose()
    return $bitmap
}

function Get-JpegBytes([Drawing.Bitmap]$bitmap, [int]$quality) {
    $codec = [Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object { $_.MimeType -eq "image/jpeg" }
    $parameters = New-Object Drawing.Imaging.EncoderParameters(1)
    $parameters.Param[0] = New-Object Drawing.Imaging.EncoderParameter([Drawing.Imaging.Encoder]::Quality, [long]$quality)
    $stream = New-Object IO.MemoryStream
    $bitmap.Save($stream, $codec, $parameters)
    return $stream.ToArray()
}

$source = [Drawing.Image]::FromFile((Resolve-Path $Image))
try {
    $portrait = Resize-Square $source $Size
    $jpeg = Get-JpegBytes $portrait $Quality
}
finally {
    $source.Dispose()
}

if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $PSScriptRoot "..\..\src\bin\Data\Interface\NpcPortraits"
}

$directory = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force $directory | Out-Null
$output = Join-Path $directory "$NpcNumber.OZJ"
[IO.File]::WriteAllBytes($output, [byte[]]($jpeg[0..($OzjHeaderLength - 1)] + $jpeg))
Write-Host "Portrait of NPC $NpcNumber written to $output ($Size x $Size)."
