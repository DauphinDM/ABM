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
        Checked 'windeployqt' @('--release', '--force-openssl', '--qmldir', "$root/src", "$root/build/tests/release/abm_tests.exe")
        $env:QT_QPA_PLATFORM = 'offscreen'
        $env:QT_QUICK_BACKEND = 'software'
        $env:QT_FORCE_STDERR_LOGGING = '1'
        $env:QT_QPA_FONTDIR = "$env:WINDIR/Fonts"
        Checked "$root/build/tests/release/abm_tests.exe" @('-nocrashhandler')
    } finally { Pop-Location }
}
New-Item -ItemType Directory -Force "$root/dist/ABM" | Out-Null
Copy-Item "$root/build/release/abm.exe" "$root/dist/ABM/ABM.exe" -Force
Copy-Item "$env:VCPKG_ROOT/installed/x64-windows/bin/*.dll" "$root/dist/ABM/" -Force
Copy-Item "$root/LICENSE" "$root/dist/ABM/LICENSE.txt" -Force
Copy-Item "$root/docs/windows.md" "$root/dist/ABM/README.txt" -Force
Copy-Item "$root/docs/THIRD-PARTY.md" "$root/dist/ABM/THIRD-PARTY.txt" -Force
# Keep vcpkg's per-dependency copyright notices with the redistributed DLLs.
$licenses = "$root/dist/ABM/licenses"
New-Item -ItemType Directory -Force $licenses | Out-Null
Get-ChildItem "$env:VCPKG_ROOT/installed/x64-windows/share" -Directory | ForEach-Object {
    $copyright = Join-Path $_.FullName 'copyright'
    if (Test-Path $copyright) {
        New-Item -ItemType Directory -Force "$licenses/$($_.Name)" | Out-Null
        Copy-Item $copyright "$licenses/$($_.Name)/copyright" -Force
    }
}
$qtVersion = (& qmake -query QT_VERSION).Trim()
if ($LASTEXITCODE -ne 0 -or $qtVersion -notmatch '^6\.\d+\.\d+$') { throw 'Cannot resolve the deployed Qt version.' }
foreach ($license in @('LGPL-3.0-only', 'GPL-3.0-only')) {
    Invoke-WebRequest "https://raw.githubusercontent.com/qt/qtbase/v$qtVersion/LICENSES/$license.txt" -OutFile "$licenses/$license.txt"
}
Checked 'windeployqt' @('--release', '--force-openssl', '--qmldir', "$root/src", '--no-translations', "$root/dist/ABM/ABM.exe")
foreach ($pattern in @('libcrypto*.dll', 'libssl*.dll', 'tls/qopensslbackend.dll')) {
    if (-not (Get-ChildItem "$root/dist/ABM/$pattern" -ErrorAction SilentlyContinue)) {
        throw "Missing bundled TLS dependency: $pattern"
    }
}
if ($Test) {
    $env:BEAMSIDE_BINARY = "$root/dist/ABM/ABM.exe"
    Checked 'python' @("$root/tests/cli_test.py")
    Checked 'python' @("$root/tests/standalone_test.py")
}
$version = (Select-String -Path "$root/pkgbuild/PKGBUILD" -Pattern '^pkgver=(.+)$').Matches.Groups[1].Value
if ($version -notmatch '^\d+\.\d+\.\d+$') { throw 'Cannot resolve the release version.' }
$zip = "$root/dist/ABM-$version-Windows-x64.zip"
Compress-Archive -Path "$root/dist/ABM" -DestinationPath $zip -Force
$hash = (Get-FileHash -Algorithm SHA256 $zip).Hash.ToLowerInvariant()
"$hash  $(Split-Path $zip -Leaf)" | Set-Content -Encoding ascii "$root/dist/SHA256SUMS-windows"
Write-Host "Standalone app: $root/dist/ABM/ABM.exe"
