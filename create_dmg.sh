#!/bin/bash
# EBOXLAB macOS DMG Installer Creator
# Run this on macOS after building with: make -f Makefile.macos
#
# Usage: ./create_dmg.sh

set -e

APP_NAME="EBOXLAB"
VERSION="1.0"
DMG_NAME="${APP_NAME}_v${VERSION}.dmg"
APP_BUNDLE="${APP_NAME}.app"
BUILD_DIR="dmg_build"

echo "=== Building EBOXLAB macOS DMG Installer ==="

# 1. Build the binary
echo "[1/5] Building eboxlab binary..."
make -f Makefile.macos clean
make -f Makefile.macos

if [ ! -f "eboxlab" ]; then
    echo "ERROR: Build failed - eboxlab binary not found"
    exit 1
fi

# 2. Create the .app bundle structure
echo "[2/5] Creating application bundle..."
rm -rf "${BUILD_DIR}" "${APP_BUNDLE}"
mkdir -p "${APP_BUNDLE}/Contents/MacOS"
mkdir -p "${APP_BUNDLE}/Contents/Resources"

# Copy binary
cp eboxlab "${APP_BUNDLE}/Contents/MacOS/eboxlab"
chmod +x "${APP_BUNDLE}/Contents/MacOS/eboxlab"

# Create a launcher script that opens Terminal with the app
cat > "${APP_BUNDLE}/Contents/MacOS/${APP_NAME}" << 'LAUNCHER'
#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
osascript -e "
tell application \"Terminal\"
    activate
    do script \"'${DIR}/eboxlab'; exit\"
end tell
"
LAUNCHER
chmod +x "${APP_BUNDLE}/Contents/MacOS/${APP_NAME}"

# Create Info.plist
cat > "${APP_BUNDLE}/Contents/Info.plist" << PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>${APP_NAME}</string>
    <key>CFBundleDisplayName</key>
    <string>EBOXLAB - Digital Forensics Platform</string>
    <key>CFBundleIdentifier</key>
    <string>com.lawstars.eboxlab</string>
    <key>CFBundleVersion</key>
    <string>${VERSION}</string>
    <key>CFBundleShortVersionString</key>
    <string>${VERSION}</string>
    <key>CFBundleExecutable</key>
    <string>${APP_NAME}</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleSignature</key>
    <string>EBLB</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
    <key>NSHumanReadableCopyright</key>
    <string>Copyright (C) 2026 Law Stars - Trial Lawyers and Legal Services Colorado LLC</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
</dict>
</plist>
PLIST

# Copy docs into Resources
cp README.md FEATURES.md PROJECT.md PROGRESS.md "${APP_BUNDLE}/Contents/Resources/" 2>/dev/null || true

# 3. Create DMG staging area
echo "[3/5] Staging DMG contents..."
mkdir -p "${BUILD_DIR}"
cp -R "${APP_BUNDLE}" "${BUILD_DIR}/"

# Add a symlink to /Applications for easy drag-install
ln -sf /Applications "${BUILD_DIR}/Applications"

# Add a README visible in the DMG
cat > "${BUILD_DIR}/README.txt" << README
EBOXLAB - Digital Forensics & eDiscovery Platform v${VERSION}
by Law Stars - Trial Lawyers and Legal Services Colorado LLC

INSTALLATION:
  Drag EBOXLAB.app to the Applications folder.

FIRST LAUNCH:
  Double-click EBOXLAB from Applications.
  It will open a Terminal window with the forensic platform.
  Demo data is automatically loaded on first launch.

DATA STORAGE:
  ~/Library/Application Support/EBOXLAB/

FEATURES:
  - Case & evidence management
  - Chain of custody tracking
  - File manager with SHA-256/MD5 hashing
  - Built for air-gapped forensic workstations
README

# 4. Create the DMG
echo "[4/5] Creating DMG..."
rm -f "${DMG_NAME}"
hdiutil create -volname "${APP_NAME}" \
    -srcfolder "${BUILD_DIR}" \
    -ov -format UDZO \
    "${DMG_NAME}"

# 5. Cleanup
echo "[5/5] Cleaning up..."
rm -rf "${BUILD_DIR}" "${APP_BUNDLE}"

echo ""
echo "=== SUCCESS ==="
echo "DMG created: $(pwd)/${DMG_NAME}"
echo "Size: $(du -h "${DMG_NAME}" | cut -f1)"
echo ""
echo "To distribute: share ${DMG_NAME}"
echo "Users drag EBOXLAB.app into Applications to install."
