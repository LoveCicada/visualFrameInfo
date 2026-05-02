param(
    [string]$Tag = '0.1.0',
    [string]$Repo = 'LoveCicada/visualFrameInfo',
    [string]$AssetPath,
    [string]$NotesFile
)

$ErrorActionPreference = 'Stop'

function Get-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
}

function Get-GhPath {
    $fromPath = Get-Command 'gh' -ErrorAction SilentlyContinue
    if ($null -ne $fromPath -and -not [string]::IsNullOrWhiteSpace($fromPath.Source)) {
        return $fromPath.Source
    }

    $fromExePath = Get-Command 'gh.exe' -ErrorAction SilentlyContinue
    if ($null -ne $fromExePath -and -not [string]::IsNullOrWhiteSpace($fromExePath.Source)) {
        return $fromExePath.Source
    }

    $candidates = @(
        'C:\Program Files\GitHub CLI\gh.exe',
        'C:\Program Files\GitHub CLI\bin\gh.exe',
        'C:\Program Files (x86)\GitHub CLI\gh.exe',
        'C:\Program Files (x86)\GitHub CLI\bin\gh.exe',
        (Join-Path $env:LOCALAPPDATA 'Programs\GitHub CLI\gh.exe'),
        (Join-Path $env:LOCALAPPDATA 'Programs\GitHub CLI\bin\gh.exe')
    )

    foreach ($candidate in $candidates) {
        if (-not [string]::IsNullOrWhiteSpace($candidate) -and (Test-Path $candidate)) {
            return (Resolve-Path $candidate).Path
        }
    }

    return $null
}

$repoRoot = Get-RepoRoot

if ([string]::IsNullOrWhiteSpace($AssetPath)) {
    $AssetPath = Join-Path $repoRoot ("install/installer/visualFrameInfo_setup_{0}.exe" -f $Tag)
}

if ([string]::IsNullOrWhiteSpace($NotesFile)) {
    $NotesFile = Join-Path $repoRoot 'docs/changes.md'
}

if (-not (Test-Path $AssetPath)) {
    throw "Installer not found: $AssetPath"
}

if (-not (Test-Path $NotesFile)) {
    throw "Release notes file not found: $NotesFile"
}

$ghPath = Get-GhPath
if ([string]::IsNullOrWhiteSpace($ghPath)) {
    throw 'gh.exe was not found in PATH. Please install GitHub CLI and run gh auth login first.'
}

Push-Location $repoRoot
try {
    & $ghPath auth status
    if ($LASTEXITCODE -ne 0) {
        throw 'GitHub CLI is not authenticated. Run gh auth login first.'
    }

    git rev-parse -q --verify "refs/tags/$Tag" *> $null
    if ($LASTEXITCODE -ne 0) {
        git tag $Tag
    }

    git push origin $Tag
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to push tag: $Tag"
    }

    & $ghPath release view $Tag --repo $Repo *> $null
    if ($LASTEXITCODE -eq 0) {
        & $ghPath release upload $Tag $AssetPath --clobber --repo $Repo
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to upload asset to existing release: $Tag"
        }

        Write-Host "Updated existing GitHub Release: $Tag"
        Write-Host "Uploaded asset: $AssetPath"
        return
    }

    & $ghPath release create $Tag --title $Tag --notes-file $NotesFile --repo $Repo
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to create GitHub Release: $Tag"
    }

    & $ghPath release upload $Tag $AssetPath --clobber --repo $Repo
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to upload asset to new release: $Tag"
    }

    Write-Host "Created GitHub Release: $Tag"
    Write-Host "Uploaded asset: $AssetPath"
}
finally {
    Pop-Location
}