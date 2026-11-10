# Windows:   .\build-windows.ps1 -GbdkHome 'C:\gbdk'
# Needs GBDK-2020 4.2 or newer (unzip it anywhere) and Python 3 on the PATH.
param(
    [string]$GbdkHome = $env:GBDK_HOME
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

if ([string]::IsNullOrWhiteSpace($GbdkHome)) {
    throw "GBDK path not provided. Run: .\build-windows.ps1 -GbdkHome 'C:\gbdk'"
}
$GbdkHome = (Resolve-Path -LiteralPath $GbdkHome).Path
$Lcc = Join-Path $GbdkHome 'bin\lcc.exe'
if (-not (Test-Path -LiteralPath $Lcc -PathType Leaf)) {
    throw "Could not find the GBDK compiler: $Lcc"
}

Push-Location $ProjectRoot
try {
    Write-Host 'Generating game art, maps and text...'
    & python tools/make_assets.py
    if ($LASTEXITCODE -ne 0) { throw "Asset generation failed (exit code $LASTEXITCODE)" }

    $Sources = Get-ChildItem -Path 'src' -Filter '*.c' -File -Recurse | Sort-Object FullName | ForEach-Object { $_.FullName }
    if ($Sources.Count -eq 0) { throw 'No C source files found under src/.' }

    $Output = Join-Path $ProjectRoot 'build\game.gb'
    $CompilerArgs = @(
        '-msm83:gb', '-Isrc', '-Wf--opt-code-size',
        '-Wl-m', '-Wl-j',
        '-Wm-ynMOONLIGHTHOLLER', '-Wm-yt0x19', '-Wm-yo4', '-Wm-yc',
        '-o', $Output
    ) + $Sources

    Write-Host "Compiling $($Sources.Count) C files with GBDK..."
    & $Lcc @CompilerArgs
    if ($LASTEXITCODE -ne 0) { throw "GBDK compilation failed (exit code $LASTEXITCODE)" }
    if (-not (Test-Path -LiteralPath $Output -PathType Leaf)) { throw "Compiler did not create $Output" }

    Write-Host 'Adding the data banks...'
    & python tools/pack_rom.py $Output build/databank.bin build/moonlight-holler.gb
    if ($LASTEXITCODE -ne 0) { throw "Packing failed (exit code $LASTEXITCODE)" }

    & python tools/verify_rom.py build/moonlight-holler.gb
    if ($LASTEXITCODE -ne 0) { throw "ROM check failed (exit code $LASTEXITCODE)" }

    Write-Host ''
    Write-Host "Done: $ProjectRoot\build\moonlight-holler.gb"
}
finally {
    Pop-Location
}
