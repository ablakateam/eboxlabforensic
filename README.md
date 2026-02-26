# EBOXLAB - Digital Forensics & eDiscovery Platform

### by Law Stars - Trial Lawyers and Legal Services Colorado LLC

---

EBOXLAB is a terminal-based digital forensics and eDiscovery platform for managing cases, evidence, chain of custody, and examiner workflows. Built in C++17, it runs natively on **Windows** and **macOS** as a single standalone executable — no database server, no internet connection, no runtime dependencies.

Designed for forensic examiners and legal professionals who need a fast, reliable, air-gapped case management tool.

---

## Features

### Case & Evidence Management
- **Clients** — Manage client organizations with contact details
- **Cases** — Track cases with status, priority, type, and assigned examiner
- **Evidence** — Catalog evidence items with acquisition details and hash values
- **Chain of Custody** — Record custody transfers with timestamps and actions
- **Examiners** — Manage forensic examiner profiles and certifications
- **Custodians** — Track data custodians linked to cases
- **Activity Log** — Automatic audit trail of all actions, sorted most-recent-first

### Forensic Tools
- **File Manager** — Norton Commander-style file browser with directory navigation, sorting by name/size/date, and drive selection
- **Hash Calculator** — SHA-256 and MD5 hash computation for any file, with full file attributes (size, timestamps, permissions)

### Platform Features
- Cross-entity search across all 7 modules
- Mouse and keyboard navigation
- Two themes: Classic Blue and Retro Green (toggle with F3)
- Demo data auto-seeded on first launch
- Binary flat-file storage — portable, no database required
- Air-gapped operation — zero network dependencies

---

## Installation

### Windows — Installer (Recommended)

1. Download `EBOXLAB_Setup_v1.0.exe` from the [Releases](../../releases) page
2. Run the installer — it creates Start Menu and Desktop shortcuts
3. Launch EBOXLAB from the shortcut

### Windows — Portable

1. Download `eboxlab.exe` from the [Releases](../../releases) page
2. Run it directly — no installation needed
3. Data is stored in `%APPDATA%\EBOXLAB\`

### macOS

1. Download `EBOXLAB_v1.0.dmg` from the [Releases](../../releases) page
2. Open the DMG and drag `EBOXLAB.app` to Applications
3. Launch from Applications — it opens in a Terminal window
4. Data is stored in `~/Library/Application Support/EBOXLAB/`

---

## Build from Source

### Windows

**Prerequisites:** MinGW-w64 (g++ with C++17), PDCurses (WinCon port)

```bash
# Install compiler
winget install BrechtSanders.WinLibs.POSIX.UCRT

# Build PDCurses
cd PDCurses/wincon
mingw32-make -f Makefile WIDE=Y

# Build EBOXLAB
cd ../..
mingw32-make
```

Produces `eboxlab.exe` — a statically linked standalone executable.

### macOS

**Prerequisites:** Xcode Command Line Tools, ncurses (via Homebrew)

```bash
# Install dependencies
xcode-select --install
brew install ncurses

# Build
NCURSES_PREFIX=$(brew --prefix ncurses)
make -f Makefile.macos \
  CXXFLAGS="-O2 -Wall -std=c++17 -I${NCURSES_PREFIX}/include" \
  LDFLAGS="-L${NCURSES_PREFIX}/lib -lncurses"
```

Produces `eboxlab` binary.

To create a DMG installer:
```bash
chmod +x create_dmg.sh
./create_dmg.sh
```

---

## Keyboard Shortcuts

### Main Menu

| Key                | Action                                    |
|--------------------|-------------------------------------------|
| **1-8**            | Quick select modules 1-8                  |
| **9**              | File Manager                              |
| **0**              | Hash Calculator                           |
| **Q**              | Exit application                          |
| **Arrow keys**     | Navigate menus and lists                  |
| **Tab / Shift+Tab**| Cycle through items                       |
| **Enter**          | Select / confirm                          |
| **Escape**         | Go back / cancel                          |
| **F2**             | Save form                                 |
| **F3**             | Toggle theme (Blue / Retro Green)         |
| **Mouse**          | Click items, scroll lists                 |

### File Manager

| Key                | Action                                    |
|--------------------|-------------------------------------------|
| **Enter**          | Open directory / view file details + hash |
| **Backspace**      | Go up one directory                       |
| **F5 / F6 / F7**  | Sort by Name / Size / Date               |
| **F8**             | Toggle ascending / descending             |
| **F9**             | Compute hash of selected file             |
| **D**              | Drive selection                           |
| **Page Up/Down**   | Scroll by page                            |
| **Home / End**     | Jump to first / last file                 |

---

## Data Storage

All data is stored in binary flat files:

| File              | Contents                  |
|-------------------|---------------------------|
| `clients.dat`     | Client records            |
| `cases.dat`       | Case records              |
| `evidence.dat`    | Evidence items            |
| `chain.dat`       | Chain of custody entries   |
| `examiners.dat`   | Examiner profiles         |
| `custodians.dat`  | Custodian records         |
| `activity.dat`    | Activity log entries      |

- **Windows:** `%APPDATA%\EBOXLAB\`
- **macOS:** `~/Library/Application Support/EBOXLAB/`

**Backup:** Copy all `.dat` files to preserve the entire database.

---

## CI/CD

Automated builds run via GitHub Actions on every push to `main`. Tagged releases (`v*`) automatically publish Windows installer, Windows portable exe, and macOS DMG to the Releases page.

---

## License

Proprietary software developed for Law Stars - Trial Lawyers and Legal Services Colorado LLC. All rights reserved.
