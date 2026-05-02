$ErrorActionPreference = 'Stop'

function Get-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
}

function Get-WindeployqtPath {
    param(
        [string]$RepoRoot
    )

    $candidates = New-Object System.Collections.Generic.List[string]

    $presetFile = Join-Path $RepoRoot 'CMakePresets.json'
    if (Test-Path $presetFile) {
        $presetJson = Get-Content -Path $presetFile -Raw | ConvertFrom-Json
        foreach ($preset in $presetJson.configurePresets) {
            if ($null -ne $preset.cacheVariables -and $null -ne $preset.cacheVariables.CMAKE_PREFIX_PATH) {
                $prefixPath = [string]$preset.cacheVariables.CMAKE_PREFIX_PATH
                if (-not [string]::IsNullOrWhiteSpace($prefixPath)) {
                    $candidates.Add((Join-Path $prefixPath 'bin/windeployqt.exe'))
                }
            }
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($env:CMAKE_PREFIX_PATH)) {
        $candidates.Add((Join-Path $env:CMAKE_PREFIX_PATH 'bin/windeployqt.exe'))
    }

    $command = Get-Command 'windeployqt.exe' -ErrorAction SilentlyContinue
    if ($null -ne $command -and -not [string]::IsNullOrWhiteSpace($command.Source)) {
        $candidates.Add($command.Source)
    }

    foreach ($candidate in $candidates) {
        if (-not [string]::IsNullOrWhiteSpace($candidate) -and (Test-Path $candidate)) {
            return (Resolve-Path $candidate).Path
        }
    }

    throw 'Unable to locate windeployqt.exe. Please verify CMAKE_PREFIX_PATH or add windeployqt.exe to PATH.'
}

$repoRoot = Get-RepoRoot
$releaseExe = Join-Path $repoRoot 'build/cmake-msvc-release/Release/visualFrameInfo.exe'
$installRoot = Join-Path $repoRoot 'install'
$deployDir = Join-Path $installRoot 'Release'

if (-not (Test-Path $releaseExe)) {
    throw "Release executable was not found: $releaseExe"
}

if (Test-Path $deployDir) {
    Remove-Item -Path $deployDir -Recurse -Force
}
New-Item -Path $deployDir -ItemType Directory -Force | Out-Null

$targetExe = Join-Path $deployDir 'visualFrameInfo.exe'
Copy-Item -Path $releaseExe -Destination $targetExe -Force

$windeployqt = Get-WindeployqtPath -RepoRoot $repoRoot
Write-Host "Using windeployqt: $windeployqt"

& $windeployqt --release --no-translations $targetExe
if ($LASTEXITCODE -ne 0) {
    throw "windeployqt failed with exit code $LASTEXITCODE"
}

$ffmpegSource = Join-Path $repoRoot 'ffmpeg'
$ffmpegTarget = Join-Path $deployDir 'ffmpeg'
if (Test-Path $ffmpegSource) {
    Copy-Item -Path $ffmpegSource -Destination $ffmpegTarget -Recurse -Force
} else {
    Write-Warning "ffmpeg directory does not exist: $ffmpegSource"
}

$fileCount = (Get-ChildItem -Path $deployDir -Recurse -File | Measure-Object).Count
Write-Host 'Deploy finished.'
Write-Host "Deploy directory: $deployDir"
Write-Host "Deployed executable: $targetExe"
Write-Host "Total files: $fileCount"
