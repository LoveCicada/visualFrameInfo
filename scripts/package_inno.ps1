param(
    [string]$AppVersion,
    [string]$OutputBaseName
)

$ErrorActionPreference = 'Stop'

function Get-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
}

function Get-InnoCompilerPath {
    $candidates = New-Object System.Collections.Generic.List[string]

    $fromPath = Get-Command 'ISCC.exe' -ErrorAction SilentlyContinue
    if ($null -ne $fromPath -and -not [string]::IsNullOrWhiteSpace($fromPath.Source)) {
        $candidates.Add($fromPath.Source)
    }

    if (-not [string]::IsNullOrWhiteSpace($env:INNO_SETUP_HOME)) {
        $candidates.Add((Join-Path $env:INNO_SETUP_HOME 'ISCC.exe'))
    }

    $candidates.Add('C:\\Program Files (x86)\\Inno Setup 6\\ISCC.exe')
    $candidates.Add('C:\\Program Files\\Inno Setup 6\\ISCC.exe')

    $registryKeys = @(
        'HKLM:\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\*',
        'HKLM:\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\*',
        'HKCU:\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\*'
    )

    foreach ($key in $registryKeys) {
        $entries = Get-ItemProperty -Path $key -ErrorAction SilentlyContinue
        foreach ($entry in $entries) {
            if ($null -ne $entry.DisplayName -and $entry.DisplayName -like 'Inno Setup*') {
                if (-not [string]::IsNullOrWhiteSpace($entry.InstallLocation)) {
                    $candidates.Add((Join-Path ([string]$entry.InstallLocation) 'ISCC.exe'))
                }
            }
        }
    }

    foreach ($candidate in $candidates) {
        if (-not [string]::IsNullOrWhiteSpace($candidate) -and (Test-Path $candidate)) {
            return (Resolve-Path $candidate).Path
        }
    }

    throw 'Unable to locate ISCC.exe. Please install Inno Setup 6 or set INNO_SETUP_HOME.'
}

function Get-ProjectVersion {
    param(
        [string]$RepoRoot
    )

    $cmakeFile = Join-Path $RepoRoot 'CMakeLists.txt'
    if (-not (Test-Path $cmakeFile)) {
        throw "CMakeLists.txt not found: $cmakeFile"
    }

    $content = Get-Content -Path $cmakeFile -Raw
    $match = [regex]::Match($content, 'project\s*\(\s*[^\s\)]+\s+VERSION\s+([0-9]+(?:\.[0-9]+){1,3})', 'IgnoreCase')
    if (-not $match.Success) {
        throw 'Unable to detect project version from CMakeLists.txt project(... VERSION ...).'
    }

    return $match.Groups[1].Value
}

function Get-PreviousInstaller {
    param(
        [string]$OutputDir,
        [string]$CurrentVersion
    )

    if (-not (Test-Path $OutputDir)) {
        return $null
    }

    $current = [version]$CurrentVersion
    $items = New-Object System.Collections.Generic.List[object]

    $files = Get-ChildItem -Path $OutputDir -Filter 'visualFrameInfo_setup_*.exe' -File -ErrorAction SilentlyContinue
    foreach ($file in $files) {
        $m = [regex]::Match($file.Name, '^visualFrameInfo_setup_([0-9]+(?:\.[0-9]+){1,3})\.exe$')
        if (-not $m.Success) {
            continue
        }

        $v = [version]$m.Groups[1].Value
        if ($v -eq $current) {
            continue
        }

        $items.Add([pscustomobject]@{
            Version = $v
            Name = $file.Name
        })
    }

    if ($items.Count -eq 0) {
        return $null
    }

    return ($items | Sort-Object -Property Version -Descending | Select-Object -First 1)
}

$repoRoot = Get-RepoRoot
$sourceDir = Join-Path $repoRoot 'install/Release'
$outDir = Join-Path $repoRoot 'install/installer'
$scriptPath = Join-Path $repoRoot 'scripts/inno/visualFrameInfo.iss'
$appVersion = $AppVersion
if ([string]::IsNullOrWhiteSpace($appVersion)) {
    $appVersion = Get-ProjectVersion -RepoRoot $repoRoot
}

if ([string]::IsNullOrWhiteSpace($OutputBaseName)) {
    $OutputBaseName = "visualFrameInfo_setup_{0}" -f $appVersion
}

$targetInstallerName = "{0}.exe" -f $OutputBaseName

if (-not (Test-Path $sourceDir)) {
    throw "Deploy source directory does not exist: $sourceDir. Run deploy_release.ps1 first."
}

if (-not (Test-Path (Join-Path $sourceDir 'visualFrameInfo.exe'))) {
    throw "Executable not found in deploy source directory: $sourceDir"
}

if (-not (Test-Path $scriptPath)) {
    throw "Inno Setup script not found: $scriptPath"
}

New-Item -Path $outDir -ItemType Directory -Force | Out-Null

$previousInstaller = Get-PreviousInstaller -OutputDir $outDir -CurrentVersion $appVersion
if ($null -ne $previousInstaller) {
    Write-Host "Installer version transition: $($previousInstaller.Name) -> $targetInstallerName"
} else {
    Write-Host "Installer version transition: (none) -> $targetInstallerName"
}

$iscc = Get-InnoCompilerPath
Write-Host "Using ISCC: $iscc"
Write-Host "Using app version: $appVersion"

& $iscc "/DSourceDir=$sourceDir" "/DOutputDir=$outDir" "/DMyAppVersion=$appVersion" "/DMyOutputBaseFilename=$OutputBaseName" $scriptPath
if ($LASTEXITCODE -ne 0) {
    throw "Inno Setup build failed with exit code $LASTEXITCODE"
}

$latest = Get-ChildItem -Path $outDir -Filter '*.exe' | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($null -eq $latest) {
    throw "No installer executable generated in: $outDir"
}

Write-Host 'Installer package finished.'
Write-Host "Output file: $($latest.FullName)"
