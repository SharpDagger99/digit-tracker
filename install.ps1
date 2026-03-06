# ============================================================
#  Digit Tracker v1.0 - Installer
#  https://github.com/YOUR_USERNAME/digit-tracker
# ============================================================

$ErrorActionPreference = "Stop"
$AppName    = "Digit Tracker v1.0"
$RepoOwner  = "SharpDagger99"
$RepoName   = "digit-tracker"
$InstallDir = "$env:LOCALAPPDATA\DigitTracker"
$CppURL     = "https://raw.githubusercontent.com/SharpDagger99/digit-tracker/main/digit.cpp"
$DataURL    = "https://raw.githubusercontent.com/SharpDagger99/digit-tracker/main/digit_data.txt"

function Write-Header {
    Clear-Host
    Write-Host ""
    Write-Host "  ==============================================" -ForegroundColor Cyan
    Write-Host "   DIGIT TRACKER v1.0  -  Installer" -ForegroundColor Cyan
    Write-Host "   Swertres 3D Lotto Analyzer" -ForegroundColor DarkCyan
    Write-Host "  ==============================================" -ForegroundColor Cyan
    Write-Host ""
}

function Write-Step($msg) {
    Write-Host "  [*] $msg" -ForegroundColor Yellow
}

function Write-OK($msg) {
    Write-Host "  [v] $msg" -ForegroundColor Green
}

function Write-Err($msg) {
    Write-Host "  [!] $msg" -ForegroundColor Red
}

# ── Step 0: Header ─────────────────────────────────────────
Write-Header

# ── Step 1: Check for MinGW / g++ ──────────────────────────
Write-Step "Checking for C++ compiler (g++)..."

$gpp = Get-Command "g++" -ErrorAction SilentlyContinue

if (-not $gpp) {
    Write-Host ""
    Write-Host "  g++ not found. Digit Tracker needs MinGW-w64 to compile." -ForegroundColor Red
    Write-Host ""
    Write-Host "  Install options:" -ForegroundColor White
    Write-Host "    [1] MSYS2 (recommended)  https://www.msys2.org" -ForegroundColor DarkCyan
    Write-Host "         After installing, run in MSYS2 terminal:" -ForegroundColor DarkGray
    Write-Host "         pacman -S mingw-w64-ucrt-x86_64-gcc" -ForegroundColor Gray
    Write-Host ""
    Write-Host "    [2] winget (if available):" -ForegroundColor DarkCyan
    Write-Host "         winget install MSYS2.MSYS2" -ForegroundColor Gray
    Write-Host ""
    Write-Host "  After installing g++, re-run this installer." -ForegroundColor Yellow
    Write-Host ""
    Read-Host "  Press Enter to exit"
    exit 1
}

Write-OK "g++ found at: $($gpp.Source)"

# ── Step 2: Create install directory ───────────────────────
Write-Step "Creating install directory..."
if (-not (Test-Path $InstallDir)) {
    New-Item -ItemType Directory -Path $InstallDir | Out-Null
}
Write-OK "Directory: $InstallDir"

# ── Step 3: Download source files ──────────────────────────
Write-Step "Downloading digit.cpp from GitHub..."

$CppDest  = "$InstallDir\digit.cpp"
$DataDest = "$InstallDir\digit_data.txt"

try {
    Invoke-WebRequest -Uri $CppURL -OutFile $CppDest -UseBasicParsing
    Write-OK "Downloaded digit.cpp"
} catch {
    Write-Err "Failed to download digit.cpp"
    Write-Host "  URL: $CppURL" -ForegroundColor DarkGray
    Write-Host "  Check your internet connection and that the repo is public." -ForegroundColor DarkGray
    Read-Host "  Press Enter to exit"
    exit 1
}

# Download digit_data.txt if it doesn't already exist
if (-not (Test-Path $DataDest)) {
    Write-Step "Downloading digit_data.txt (draw history)..."
    try {
        Invoke-WebRequest -Uri $DataURL -OutFile $DataDest -UseBasicParsing
        Write-OK "Downloaded digit_data.txt"
    } catch {
        Write-Host "  [i] digit_data.txt not found in repo — will be created on first Sync." -ForegroundColor DarkGray
        New-Item -ItemType File -Path $DataDest | Out-Null
    }
} else {
    Write-Host "  [i] digit_data.txt already exists — keeping your data." -ForegroundColor DarkGray
}

# ── Step 4: Compile ────────────────────────────────────────
Write-Step "Compiling digit.cpp (this takes a few seconds)..."

$ExeDest = "$InstallDir\digit.exe"
$CompileArgs = @(
    "-std=c++17", "-O2",
    "-o", $ExeDest,
    $CppDest,
    "-lwinhttp"
)

try {
    $proc = Start-Process -FilePath "g++" -ArgumentList $CompileArgs `
                          -Wait -PassThru -NoNewWindow `
                          -RedirectStandardError "$InstallDir\compile_err.txt"
    if ($proc.ExitCode -ne 0) {
        $errText = Get-Content "$InstallDir\compile_err.txt" -Raw
        Write-Err "Compilation failed (exit code $($proc.ExitCode))"
        Write-Host ""
        Write-Host $errText -ForegroundColor DarkRed
        Read-Host "  Press Enter to exit"
        exit 1
    }
    Remove-Item "$InstallDir\compile_err.txt" -ErrorAction SilentlyContinue
    Write-OK "Compiled successfully -> digit.exe"
} catch {
    Write-Err "Could not run g++: $_"
    Read-Host "  Press Enter to exit"
    exit 1
}

# ── Step 5: Create shortcut on Desktop ─────────────────────
Write-Step "Creating Desktop shortcut..."

try {
    $WshShell  = New-Object -ComObject WScript.Shell
    $Shortcut  = $WshShell.CreateShortcut("$env:USERPROFILE\Desktop\Digit Tracker.lnk")
    $Shortcut.TargetPath       = $ExeDest
    $Shortcut.WorkingDirectory = $InstallDir
    $Shortcut.Description      = "Digit Tracker v1.3 - Swertres 3D Analyzer"
    $Shortcut.Save()
    Write-OK "Desktop shortcut created"
} catch {
    Write-Host "  [i] Could not create shortcut (non-fatal): $_" -ForegroundColor DarkGray
}

# ── Step 6: Add to PATH (user-level) ───────────────────────
Write-Step "Adding to user PATH..."

$CurrentPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($CurrentPath -notlike "*$InstallDir*") {
    [Environment]::SetEnvironmentVariable("Path", "$CurrentPath;$InstallDir", "User")
    Write-OK "Added to PATH — you can run 'digit' from any terminal after restarting it"
} else {
    Write-Host "  [i] Already in PATH" -ForegroundColor DarkGray
}

# ── Done ───────────────────────────────────────────────────
Write-Host ""
Write-Host "  ==============================================" -ForegroundColor Green
Write-Host "   INSTALLATION COMPLETE!" -ForegroundColor Green
Write-Host "  ==============================================" -ForegroundColor Green
Write-Host ""
Write-Host "  Installed to : $InstallDir" -ForegroundColor White
Write-Host "  Run with     : digit" -ForegroundColor Cyan
Write-Host "              or double-click 'Digit Tracker' on Desktop" -ForegroundColor DarkGray
Write-Host ""
Write-Host "  First run tip:" -ForegroundColor Yellow
Write-Host "  Use [7] Sync DB or [8] Import CSV to load draw history." -ForegroundColor DarkGray
Write-Host ""

$launch = Read-Host "  Launch Digit Tracker now? (Y/N)"
if ($launch -match "^[Yy]") {
    Start-Process -FilePath $ExeDest -WorkingDirectory $InstallDir
}
