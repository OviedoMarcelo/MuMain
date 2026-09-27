<#
.SYNOPSIS
    Converts the client's slide help file (Data\Local\<Lang>\slide_<lang>.bmd) to JSON and back.

.DESCRIPTION
    The slide help file holds the tips that scroll across the top of the screen. It is the
    SLIDEHELP struct from src/source/UI/Legacy/UIControls.h, XOR-encrypted with BuxConvert:

        int32   iCreateDelay                 seconds between two tips
        float32 fSpeed                       scroll speed
        5 x {                                one group per level range
            int32 iLevel                     the group applies up to this character level
            int32 iNumber                    how many of the 32 texts are used
            char  szSlideHelpText[32][256]   NUL-terminated UTF-8 texts
        }

    Export also accepts texts saved as Windows-1252; import always writes UTF-8.

.EXAMPLE
    .\slide-bmd.ps1 export ..\src\bin\Data\Local\Spn\slide_spn.bmd slide_spn.json
    # edit slide_spn.json
    .\slide-bmd.ps1 import slide_spn.json ..\src\bin\Data\Local\Spn\slide_spn.bmd
#>
param(
    [Parameter(Mandatory = $true, Position = 0)][ValidateSet('export', 'import')][string]$Mode,
    [Parameter(Mandatory = $true, Position = 1)][string]$Source,
    [Parameter(Mandatory = $true, Position = 2)][string]$Destination
)

$ErrorActionPreference = 'Stop'

$GroupCount = 5
$TextCount = 32
$TextLength = 256
$GroupSize = 8 + $TextCount * $TextLength
$FileSize = 8 + $GroupCount * $GroupSize
$BuxCode = [byte[]](0xFC, 0xCF, 0xAB)
$Utf8 = New-Object System.Text.UTF8Encoding($false)
$StrictUtf8 = New-Object System.Text.UTF8Encoding($false, $true)
# The shipped Spn/Por files were saved as Windows-1252 although the client reads UTF-8.
$Legacy = [System.Text.Encoding]::GetEncoding(1252)

function Get-Text([byte[]]$Buffer, [int]$Start, [int]$Count) {
    try {
        $StrictUtf8.GetString($Buffer, $Start, $Count)
    }
    catch [System.ArgumentException] {
        $Legacy.GetString($Buffer, $Start, $Count)
    }
}

function Convert-Bux([byte[]]$Buffer) {
    for ($i = 0; $i -lt $Buffer.Length; $i++) {
        $Buffer[$i] = $Buffer[$i] -bxor $BuxCode[$i % 3]
    }
}

function Resolve-OutputPath([string]$Path) {
    $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Path)
}

function Export-SlideHelp {
    $data = [System.IO.File]::ReadAllBytes((Resolve-Path $Source))
    if ($data.Length -ne $FileSize) {
        throw "$Source has $($data.Length) bytes, expected $FileSize."
    }
    Convert-Bux $data

    $groups = @()
    for ($g = 0; $g -lt $GroupCount; $g++) {
        $offset = 8 + $g * $GroupSize
        $number = [BitConverter]::ToInt32($data, $offset + 4)
        if ($number -lt 0 -or $number -gt $TextCount) {
            throw "Group $g says it has $number texts (maximum $TextCount)."
        }
        $texts = @()
        for ($t = 0; $t -lt $number; $t++) {
            $start = $offset + 8 + $t * $TextLength
            $end = [Array]::IndexOf($data, [byte]0, $start, $TextLength)
            if ($end -lt 0) { $end = $start + $TextLength }
            $texts += Get-Text $data $start ($end - $start)
        }
        $groups += [ordered]@{
            maxLevel = [BitConverter]::ToInt32($data, $offset)
            texts    = $texts
        }
    }

    $json = [ordered]@{
        createDelay = [BitConverter]::ToInt32($data, 0)
        speed       = [BitConverter]::ToSingle($data, 4)
        groups      = $groups
    } | ConvertTo-Json -Depth 4
    [System.IO.File]::WriteAllText((Resolve-OutputPath $Destination), $json, $Utf8)
}

function Import-SlideHelp {
    $json = [System.IO.File]::ReadAllText((Resolve-Path $Source), $Utf8) | ConvertFrom-Json
    $groups = @($json.groups)
    if ($groups.Count -ne $GroupCount) {
        throw "Expected exactly $GroupCount groups, found $($groups.Count)."
    }

    $data = New-Object byte[] $FileSize
    [BitConverter]::GetBytes([int]$json.createDelay).CopyTo($data, 0)
    [BitConverter]::GetBytes([single]$json.speed).CopyTo($data, 4)

    for ($g = 0; $g -lt $GroupCount; $g++) {
        $offset = 8 + $g * $GroupSize
        $texts = @($groups[$g].texts | Where-Object { $null -ne $_ })
        if ($texts.Count -gt $TextCount) {
            throw "Group $g has $($texts.Count) texts (maximum $TextCount)."
        }
        [BitConverter]::GetBytes([int]$groups[$g].maxLevel).CopyTo($data, $offset)
        [BitConverter]::GetBytes([int]$texts.Count).CopyTo($data, $offset + 4)
        for ($t = 0; $t -lt $texts.Count; $t++) {
            $bytes = $Utf8.GetBytes([string]$texts[$t])
            if ($bytes.Length -ge $TextLength) {
                throw "Group $g, text $t is $($bytes.Length) bytes long in UTF-8 (maximum $($TextLength - 1)): $($texts[$t])"
            }
            $bytes.CopyTo($data, $offset + 8 + $t * $TextLength)
        }
    }

    Convert-Bux $data
    [System.IO.File]::WriteAllBytes((Resolve-OutputPath $Destination), $data)
}

if ($Mode -eq 'export') { Export-SlideHelp } else { Import-SlideHelp }
