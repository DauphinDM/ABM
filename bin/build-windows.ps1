param([switch]$Test)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (-not $env:VCPKG_ROOT) { throw 'Set VCPKG_ROOT to a vcpkg checkout with libarchive:x64-windows installed.' }
function Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed ($LASTEXITCODE)" }
}
New-Item -ItemType Directory -Force "$root/build" | Out-Null
Push-Location "$root/build"
try {
    Checked 'qmake' @("$root/abm.pro", 'CONFIG+=release', 'CONFIG-=debug')
    Checked 'nmake' @('/NOLOGO')
} finally { Pop-Location }
if ($Test) {
    New-Item -ItemType Directory -Force "$root/build/tests" | Out-Null
    Push-Location "$root/build/tests"
    try {
        Checked 'qmake' @("$root/tests/abm_tests.pro", 'CONFIG+=release', 'CONFIG-=debug')
        Checked 'nmake' @('/NOLOGO')
        Copy-Item "$env:VCPKG_ROOT/installed/x64-windows/bin/*.dll" "$root/build/tests/release/" -Force
        Checked 'windeployqt' @('--release', '--qmldir', "$root/src", "$root/build/tests/release/abm_tests.exe")
        $env:QT_QPA_PLATFORM = 'offscreen'
        $env:QT_QUICK_BACKEND = 'software'
        $env:QT_FORCE_STDERR_LOGGING = '1'
        Checked "$root/build/tests/release/abm_tests.exe" @('-nocrashhandler')
    } finally { Pop-Location }
}
New-Item -ItemType Directory -Force "$root/dist/ABM" | Out-Null
Copy-Item "$root/build/release/abm.exe" "$root/dist/ABM/ABM.exe" -Force
Copy-Item "$env:VCPKG_ROOT/installed/x64-windows/bin/*.dll" "$root/dist/ABM/" -Force
Copy-Item "$root/LICENSE" "$root/dist/ABM/LICENSE.txt" -Force
Copy-Item "$root/docs/windows.md" "$root/dist/ABM/README.txt" -Force
Checked 'windeployqt' @('--release', '--qmldir', "$root/src", '--no-translations', "$root/dist/ABM/ABM.exe")
if ($Test) {
    $env:BEAMSIDE_BINARY = "$root/dist/ABM/ABM.exe"
    Checked 'python' @("$root/tests/cli_test.py")
    Checked 'python' @("$root/tests/standalone_test.py")
}
Write-Host "Standalone app: $root/dist/ABM/ABM.exe"
