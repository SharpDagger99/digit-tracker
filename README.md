# Digit Tracker v1.0
### Swertres 3D Lotto Analyzer for Windows

A terminal-based Windows app for tracking, analyzing, and predicting Philippine Swertres (3D Lotto) draw results. Fetches live data from [lottopcso.com](https://www.lottopcso.com), supports CSV import, full draw history sync, probability analysis, and a unique **Last Digit** algorithm.

---

## ⚡ One-Line Install (Windows PowerShell)

Open **PowerShell** (not CMD) and paste:

```powershell
irm https://raw.githubusercontent.com/YOUR_USERNAME/digit-tracker/main/install.ps1 | iex
```

> **Requires:** Windows 10/11 + [MinGW-w64 (g++)](https://www.msys2.org)  
> If you don't have g++ yet, the installer will tell you how to get it.

---

## 📦 What Gets Installed

| File | Location |
|------|----------|
| `digit.exe` | `%LOCALAPPDATA%\DigitTracker\` |
| `digit.cpp` | `%LOCALAPPDATA%\DigitTracker\` |
| `digit_data.txt` | `%LOCALAPPDATA%\DigitTracker\` |
| Desktop shortcut | `%USERPROFILE%\Desktop\` |

After install, type `digit` in any terminal to launch.

---

## 🎮 Features

| # | Feature | Description |
|---|---------|-------------|
| 1 | **Search** | Find a draw result by date & time — local or online |
| 2 | **Insert/Edit** | Manually add or update a local entry |
| 3 | **Show All** | Browse full history — local or fetched live |
| 4 | **Probability** | How often a combo appears — with year-sorted results |
| 5 | **Last Digit** | Find combos gone longest without repeating (999-draw window) |
| 6 | **Browse** | View current year results live from lottopcso.com |
| 7 | **Sync DB** | Fetch any year range (2009–present) into local DB |
| 8 | **Import CSV** | Load results from a CSV file into local DB |
| 9 | **Delete** | Remove a local entry |

---

## 🔢 Last Digit Algorithm

Takes the last **999 draws** starting from a chosen point:
- Each combo is counted **once** — only its most recent appearance in the window
- Ranks unique combos from most recently drawn (#1) to least recently drawn (last)
- The **bottom N** = combos that have gone the longest without repeating = the "last digits"
- **Auto Find** starts from the current draw
- **Starting Point** lets you pick any historical date & time as the window start

---

## 📁 CSV Format

If you have a historical CSV file, it must follow this format:

```
ID,DD.MM.YYYY,HH:MM,d1,d2,d3
00001,02.01.2007,11:00,05,08,07
00002,02.01.2007,21:00,03,06,07
```

| Field | Format | Example |
|-------|--------|---------|
| ID | Any number | `00001` |
| Date | `DD.MM.YYYY` | `04.03.2026` |
| Time | `11:00` / `14:00` / `16:00` / `17:00` / `21:00` | `14:00` |
| d1, d2, d3 | Single digits (leading zeros OK) | `05,09,02` → `5-9-2` |

Time mapping: `11:00`/`14:00` → 2pm · `16:00`/`17:00` → 5pm · `21:00` → 9pm

---

## 🛠️ Manual Build

If you want to compile yourself:

```bash
# Clone the repo
git clone https://github.com/SharpDagger99/digit-tracker.git
cd digit-tracker

# Compile (requires MinGW-w64 with WinHTTP)
g++ -std=c++17 -O2 -o digit.exe digit.cpp -lwinhttp

# Run
digit.exe
```

---

## 📋 Requirements

- **OS:** Windows 10 or 11
- **Compiler:** [MinGW-w64](https://www.msys2.org) with `g++`
  ```
  # In MSYS2 terminal:
  pacman -S mingw-w64-ucrt-x86_64-gcc
  ```
- **Internet:** Required for Sync DB, Browse, and Online Search/Probability
- **WinHTTP:** Included in Windows — no extra install needed

---

## 📂 Repo Structure

```
digit-tracker/
├── digit.cpp          ← Full source code
├── digit_data.txt     ← Local draw history (pipe-separated)
├── install.ps1        ← One-line PowerShell installer
└── README.md
```

---

## 🗒️ digit_data.txt Format

```
Mar 5, 2026|2pm|0-6-4
Mar 5, 2026|5pm|5-0-9
Mar 4, 2026|2pm|5-9-2
```

Each line: `Date|Time|Combo`

---

## 📜 Uninstall

```powershell
Remove-Item "$env:LOCALAPPDATA\DigitTracker" -Recurse -Force
Remove-Item "$env:USERPROFILE\Desktop\Digit Tracker.lnk" -Force
```

Then remove `%LOCALAPPDATA%\DigitTracker` from your user PATH in System Settings.

---


*Data sourced from [lottopcso.com](https://www.lottopcso.com). For personal use only.*
