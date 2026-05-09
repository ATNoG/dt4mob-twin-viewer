param(
    [ValidateSet("Development", "DebugGame", "Shipping")]
    [string]$Config = "Development",
    [switch]$Editor,
    [switch]$Rebuild,
    [switch]$Help
)

if ($Help) {
    Write-Host ""
    Write-Host "Usage: .\rebuild.ps1 [-Config <config>] [-Editor] [-Rebuild] [-Help]" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -Config   Build configuration. Default: Development"
    Write-Host "            Values: Development | DebugGame | Shipping"
    Write-Host "  -Editor   Build the Editor target (DT4MOBEditor) and launch it on success"
    Write-Host "  -Rebuild  Delete Binaries/ and Intermediate/, regen project files, then build"
    Write-Host "  -Help     Show this help message"
    Write-Host ""
    Write-Host "Examples:"
    Write-Host "  .\rebuild.ps1                          # Incremental build, Development"
    Write-Host "  .\rebuild.ps1 -Editor                  # Build + open editor, Development"
    Write-Host "  .\rebuild.ps1 -Config DebugGame        # Incremental build, DebugGame"
    Write-Host "  .\rebuild.ps1 -Rebuild                 # Clean rebuild, Development"
    Write-Host "  .\rebuild.ps1 -Editor -Rebuild         # Clean rebuild + open editor"
    Write-Host ""
    exit 0
}

$UE        = "C:\Program Files\Epic Games\UE_5.6"
$Project   = "$PSScriptRoot\DT4MOB.uproject"
$UBTExe    = "$UE\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
$BuildBat  = "$UE\Engine\Build\BatchFiles\Build.bat"
$EditorExe = "$UE\Engine\Binaries\Win64\UnrealEditor.exe"

$Target = if ($Editor) { "DT4MOBEditor" } else { "DT4MOB" }

if ($Rebuild) {
    Write-Host "Deleting Binaries..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "$PSScriptRoot\Binaries" -ErrorAction SilentlyContinue

    Write-Host "Deleting Intermediate..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "$PSScriptRoot\Intermediate" -ErrorAction SilentlyContinue

    Write-Host "Deleting Build..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "$PSScriptRoot\Build" -ErrorAction SilentlyContinue

    Write-Host "Deleting DerivedDataCache..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "$PSScriptRoot\DerivedDataCache" -ErrorAction SilentlyContinue

    Write-Host "Regenerating project files..." -ForegroundColor Cyan
    & $UBTExe -projectfiles -project="$Project" -game -rocket -progress
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Project file generation FAILED (exit $LASTEXITCODE)" -ForegroundColor Red
        exit $LASTEXITCODE
    }
}

Write-Host "[Build] $Target Win64 $Config" -ForegroundColor Cyan

& $BuildBat $Target Win64 $Config $Project -waitmutex

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build FAILED (exit $LASTEXITCODE)" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "Build succeeded." -ForegroundColor Green

if ($Editor) {
    Write-Host "Launching editor..." -ForegroundColor Cyan
    Start-Process $EditorExe -ArgumentList "`"$Project`""
}
