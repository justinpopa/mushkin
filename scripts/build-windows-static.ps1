# build-windows-static.ps1 - Release build for Windows with static Qt
#
# Builds a native binary with Qt statically linked. Other dependencies
# (pcre, luajit, sqlite, openssl) are linked dynamically from vcpkg.
#
# Prerequisites:
#   - Visual Studio 2022 with C++ workload
#   - Git for Windows
#   - Python 3.x (for aqtinstall)
#   - vcpkg (https://github.com/microsoft/vcpkg)
#
#   # Install vcpkg (one-time setup)
#   git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
#   C:\vcpkg\bootstrap-vcpkg.bat
#   setx VCPKG_ROOT "C:\vcpkg"
#
#   # Install dependencies
#   C:\vcpkg\vcpkg install pcre:x64-windows luajit:x64-windows sqlite3:x64-windows openssl:x64-windows zlib:x64-windows
#
#   # Install aqtinstall
#   pip install aqtinstall
#
# Usage:
#   .\scripts\build-windows-static.ps1
#
# Output:
#   build-static\mushkin.exe
#
# Note: First run takes ~60 minutes to build Qt.
#       Subsequent builds take ~5 minutes (Qt is cached).

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $ProjectRoot "build-static"
$CacheDir = Join-Path $env:LOCALAPPDATA "mushkin"
$QtStaticDir = Join-Path $CacheDir "Qt-static\6.9.3-static"

# Colors for output
function Write-Info { Write-Host "[INFO] $args" -ForegroundColor Green }
function Write-Warn { Write-Host "[WARN] $args" -ForegroundColor Yellow }
function Write-Err { Write-Host "[ERROR] $args" -ForegroundColor Red }

$NumCores = (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors

# ============================================================
# Prerequisites
# ============================================================

function Check-Prerequisites {
    Write-Info "Checking prerequisites..."
    $missing = @()

    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { $missing += "cmake" }
    if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) { $missing += "ninja" }
    if (-not (Get-Command aqt -ErrorAction SilentlyContinue)) { $missing += "aqtinstall (pip install aqtinstall)" }

    # Check VCPKG_ROOT
    if (-not $env:VCPKG_ROOT -or -not (Test-Path $env:VCPKG_ROOT)) {
        $missing += "vcpkg (set VCPKG_ROOT environment variable)"
    }

    # Check vcpkg packages
    $vcpkgInstalled = Join-Path $env:VCPKG_ROOT "installed\x64-windows"
    if (Test-Path $vcpkgInstalled) {
        if (-not (Test-Path (Join-Path $vcpkgInstalled "lib\pcre.lib"))) { $missing += "pcre:x64-windows" }
        if (-not (Test-Path (Join-Path $vcpkgInstalled "lib\lua51.lib"))) { $missing += "luajit:x64-windows" }
        if (-not (Test-Path (Join-Path $vcpkgInstalled "lib\sqlite3.lib"))) { $missing += "sqlite3:x64-windows" }
        if (-not (Test-Path (Join-Path $vcpkgInstalled "lib\libssl.lib"))) { $missing += "openssl:x64-windows" }
    } else {
        $missing += "vcpkg packages not installed"
    }

    if ($missing.Count -gt 0) {
        Write-Err "Missing: $($missing -join ', ')"
        Write-Err "Install vcpkg packages with:"
        Write-Err "  $env:VCPKG_ROOT\vcpkg install pcre:x64-windows luajit:x64-windows sqlite3:x64-windows openssl:x64-windows zlib:x64-windows"
        exit 1
    }
    Write-Info "Prerequisites OK"
}

# ============================================================
# Static Qt Build
# ============================================================

function Qt-IsBuilt {
    $qtCore = Join-Path $QtStaticDir "lib\Qt6Core.lib"
    return (Test-Path $qtCore)
}

function Build-StaticQt {
    if (Qt-IsBuilt) {
        Write-Info "Static Qt already built"
        return
    }

    Write-Info "Building static Qt (~60 min)..."

    $QtVersion = "6.9.3"
    $QtBaseDir = Join-Path $CacheDir "Qt-static"
    $QtSrcDir = Join-Path $QtBaseDir "$QtVersion\Src"
    $QtBuildDir = Join-Path $QtSrcDir "build-static"

    # Download Qt source
    if (-not (Test-Path $QtSrcDir)) {
        Write-Info "Downloading Qt source..."
        New-Item -ItemType Directory -Force -Path $QtBaseDir | Out-Null
        Push-Location $QtBaseDir
        aqt install-src windows $QtVersion --outputdir $QtBaseDir
        Pop-Location
    }

    # Configure
    New-Item -ItemType Directory -Force -Path $QtBuildDir | Out-Null
    Push-Location $QtBuildDir

    $configureCmd = Join-Path $QtSrcDir "configure.bat"
    $vcpkgInstalled = Join-Path $env:VCPKG_ROOT "installed\x64-windows"

    & $configureCmd `
        -static `
        -release `
        -prefix $QtStaticDir `
        -system-sqlite `
        -skip qtwebengine -skip qtwebview -skip qt3d -skip qtcharts `
        -skip qtdatavis3d -skip qtlottie -skip qtquick3d -skip qtquick3dphysics `
        -skip qtvirtualkeyboard -skip qtwayland -skip qtwebchannel -skip qtwebsockets `
        -skip qtpositioning -skip qtsensors -skip qtserialport -skip qtserialbus `
        -skip qtremoteobjects -skip qthttpserver -skip qtquicktimeline `
        -skip qtquickeffectmaker -skip qtlocation -skip qtcoap -skip qtmqtt `
        -skip qtopcua -skip qtgrpc -skip qtlanguageserver -skip qtspeech `
        -skip qtconnectivity -skip qtactiveqt -skip qtscxml `
        -nomake examples -nomake tests `
        -- -DSQLite3_ROOT="$vcpkgInstalled" -DCMAKE_OBJECT_PATH_MAX=350

    if ($LASTEXITCODE -ne 0) { throw "Qt configure failed" }

    # Build & install
    cmake --build . --parallel $NumCores
    if ($LASTEXITCODE -ne 0) { throw "Qt build failed" }

    cmake --install .
    if ($LASTEXITCODE -ne 0) { throw "Qt install failed" }

    Pop-Location
    Write-Info "Static Qt built successfully"
}

# ============================================================
# Build Mushkin
# ============================================================

function Build-Mushkin {
    Write-Info "Building mushkin..."

    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
    }
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
    Push-Location $BuildDir

    $vcpkgToolchain = Join-Path $env:VCPKG_ROOT "scripts\buildsystems\vcpkg.cmake"

    cmake $ProjectRoot `
        -G Ninja `
        -DSTATIC_BUILD=ON `
        -DCMAKE_BUILD_TYPE=Release `
        -DCMAKE_PREFIX_PATH="$QtStaticDir" `
        -DCMAKE_TOOLCHAIN_FILE="$vcpkgToolchain" `
        -DVCPKG_TARGET_TRIPLET=x64-windows

    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

    cmake --build . --target mushkin -j $NumCores
    if ($LASTEXITCODE -ne 0) { throw "Build failed" }

    Pop-Location
}

# ============================================================
# Verify Build
# ============================================================

function Verify-Build {
    $binary = Join-Path $BuildDir "mushkin.exe"

    if (-not (Test-Path $binary)) {
        Write-Err "Build failed - binary not found"
        exit 1
    }

    Write-Host ""
    Write-Info "=== Build Summary ==="
    $size = (Get-Item $binary).Length / 1MB
    Write-Host "  Size: $([math]::Round($size, 1)) MB"
    Write-Host "  Location: $binary"
    Write-Host ""

    # Show DLL dependencies (requires dumpbin from VS)
    $dumpbin = Get-Command dumpbin -ErrorAction SilentlyContinue
    if ($dumpbin) {
        Write-Info "DLL dependencies:"
        & dumpbin /dependents $binary | Select-String "\.dll" | Select-Object -First 20
    }
}

# ============================================================
# Main
# ============================================================

function Main {
    Write-Host "=========================================="
    Write-Host "  Mushkin Release Build (Windows x64)"
    Write-Host "=========================================="
    Write-Host ""

    Check-Prerequisites
    Build-StaticQt
    Build-Mushkin
    Verify-Build

    Write-Host ""
    Write-Info "Build complete!"
    Write-Info "Binary: $(Join-Path $BuildDir 'mushkin.exe')"
}

# Run from VS Developer PowerShell for proper environment
if (-not $env:VSINSTALLDIR) {
    Write-Warn "Not running in Visual Studio Developer environment."
    Write-Warn "Run from 'Developer PowerShell for VS 2022' for best results."
}

Main
