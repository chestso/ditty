#!/bin/bash
# make-dist-app.sh — assemble a portable macOS .app bundle of ditty.
#
# Produces a ZIP containing a .app bundle that runs without Homebrew installed.
# Layout inside the ZIP:
#
#   ditty-<version>-macos-<arch>/
#     Ditty.app/
#       Contents/
#         Info.plist
#         PkgInfo
#         MacOS/
#           ditty          — wrapper script (sets DITTY_LISP_PATH, then runs real)
#           ditty.real     — REPL executable
#           flare          — syntax highlighter
#         Frameworks/
#           *.dylib        — all runtime dylib dependencies
#         Resources/
#           share/ditty/lisp/lisp-fmt.lisp
#           share/ditty/lisp/tui-events.lisp
#           share/emacs/site-lisp/ditty-mode.el
#           share/icons/hicolor/scalable/apps/ditty.svg
#           ditty.icns
#     README-PORTABLE.txt
#
# Usage (invoked from the top-level Makefile):
#   make dist-app
#
# Or directly:
#   scripts/make-dist-app.sh [BUILD_DIR] [SRC_DIR]
#
# BUILD_DIR defaults to "build", SRC_DIR defaults to the repo root.
# The resulting file is ditty-<version>-macos-<arch>.zip.

set -eu

# Resolve paths
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="${2:-$(cd "$SCRIPT_DIR/.." && pwd)}"
BUILD_DIR="${1:-$SRC_DIR/build}"

# Get version from the same script configure uses
VERSION="$("$SRC_DIR/build-aux/git-version.sh" "$SRC_DIR" 2>/dev/null || echo "0.0.0-unknown")"

# --- Binaries ---------------------------------------------------------------
# libtool wraps the real binary in .libs/ — use that if it exists.
REAL_DITTY="$BUILD_DIR/cli/ditty"
if [ -f "$BUILD_DIR/cli/.libs/ditty" ]; then
	REAL_DITTY="$BUILD_DIR/cli/.libs/ditty"
fi

REAL_FLARE="$BUILD_DIR/flare/flare"
if [ -f "$BUILD_DIR/flare/.libs/flare" ]; then
	REAL_FLARE="$BUILD_DIR/flare/.libs/flare"
fi

# Detect target architecture from the built binary
ARCH=$(file "$REAL_DITTY" 2>/dev/null | grep -oq 'arm64\|aarch64' && echo "arm64" || echo "x86_64")
PKG_NAME="ditty-${VERSION}-macos-${ARCH}"
STAGE_DIR="$SRC_DIR/$PKG_NAME"
APP_DIR="$STAGE_DIR/Ditty.app"
ZIP_FILE="$SRC_DIR/${PKG_NAME}.zip"

echo "==> Packaging ditty $VERSION (macOS $ARCH)"

# Fresh staging directory
rm -rf "$STAGE_DIR"
mkdir -p "$APP_DIR/Contents/MacOS" \
	"$APP_DIR/Contents/Frameworks" \
	"$APP_DIR/Contents/Resources/share/ditty/lisp" \
	"$APP_DIR/Contents/Resources/share/emacs/site-lisp" \
	"$APP_DIR/Contents/Resources/share/icons/hicolor/scalable/apps"

# --- Binaries ---------------------------------------------------------------
echo "==> Copying ditty (from $REAL_DITTY)"
cp "$REAL_DITTY" "$APP_DIR/Contents/MacOS/ditty.real"

echo "==> Copying flare (from $REAL_FLARE)"
cp "$REAL_FLARE" "$APP_DIR/Contents/MacOS/flare"

# --- Bundle dylibs ---------------------------------------------------------
echo "==> Bundling dylibs"
DYLIB_DIR="$APP_DIR/Contents/Frameworks"

# System libraries that ship with macOS and don't need bundling.
# libSystem covers libc/libm/libpthread, libobjc is the ObjC runtime, etc.
SYSTEM_LIBS='libSystem\.|libc\.\|libobjc\.\|libiconv\.\|libcharset\.|libc\+\+'

SEEN="$(mktemp)"
trap 'rm -f "$SEEN"' EXIT

bundle_dylib() {
	local lib_path="$1"
	local base
	base="$(basename "$lib_path")"

	# Skip system libraries
	if echo "$base" | grep -qE "^($SYSTEM_LIBS)"; then
		return
	fi

	# Skip if already bundled
	if grep -qxF "$base" "$SEEN" 2>/dev/null; then
		return
	fi
	echo "$base" >>"$SEEN"

	echo "  bundling: $base"
	cp "$lib_path" "$DYLIB_DIR/$base"
	chmod 755 "$DYLIB_DIR/$base"

	# Fix install name in the dylib itself so it finds its siblings
	install_name_tool -id "@rpath/$base" "$DYLIB_DIR/$base" 2>/dev/null || true

	# Recurse into the dylib's own dependencies
	local deps
	deps=$(otool -L "$lib_path" 2>/dev/null | tail -n +2 | awk '{print $1}')
	for dep in $deps; do
		[ -f "$dep" ] || continue
		bundle_dylib "$dep"
	done
}

# Process the main binaries' dependencies
for bin in "$REAL_DITTY" "$REAL_FLARE"; do
	deps=$(otool -L "$bin" 2>/dev/null | tail -n +2 | awk '{print $1}')
	for dep in $deps; do
		[ -f "$dep" ] || continue
		bundle_dylib "$dep"
	done
done

# Fix install names in the main binaries to point to @rpath
for bin in "$APP_DIR/Contents/MacOS/ditty.real" "$APP_DIR/Contents/MacOS/flare"; do
	for dep in $(otool -L "$bin" 2>/dev/null | tail -n +2 | awk '{print $1}'); do
		base="$(basename "$dep")"
		if [ -f "$DYLIB_DIR/$base" ]; then
			install_name_tool -change "$dep" "@rpath/$base" "$bin" 2>/dev/null || true
		fi
	done
	# Add LC_RPATH pointing to the Frameworks directory
	install_name_tool -add_rpath "@executable_path/../Frameworks" "$bin" 2>/dev/null || true
done

# --- Data files ------------------------------------------------------------
echo "==> Copying data files (lisp, emacs, icons)"
if [ -f "$SRC_DIR/lisp/lisp-fmt.lisp" ]; then
	cp "$SRC_DIR/lisp/lisp-fmt.lisp" \
		"$APP_DIR/Contents/Resources/share/ditty/lisp/lisp-fmt.lisp"
fi
if [ -f "$SRC_DIR/lisp/tui-events.lisp" ]; then
	cp "$SRC_DIR/lisp/tui-events.lisp" \
		"$APP_DIR/Contents/Resources/share/ditty/lisp/tui-events.lisp"
fi
if [ -f "$SRC_DIR/emacs/ditty-mode.el" ]; then
	cp "$SRC_DIR/emacs/ditty-mode.el" \
		"$APP_DIR/Contents/Resources/share/emacs/site-lisp/ditty-mode.el"
fi
if [ -f "$SRC_DIR/icons/hicolor/scalable/apps/ditty.svg" ]; then
	cp "$SRC_DIR/icons/hicolor/scalable/apps/ditty.svg" \
		"$APP_DIR/Contents/Resources/share/icons/hicolor/scalable/apps/ditty.svg"
fi

# --- Info.plist ------------------------------------------------------------
echo "==> Writing Info.plist"
cat >"$APP_DIR/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>Ditty</string>
    <key>CFBundleDisplayName</key>
    <string>Ditty</string>
    <key>CFBundleIdentifier</key>
    <string>so.chestso.ditty</string>
    <key>CFBundleVersion</key>
    <string>${VERSION}</string>
    <key>CFBundleShortVersionString</key>
    <string>${VERSION}</string>
    <key>CFBundleExecutable</key>
    <string>ditty</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleIconFile</key>
    <string>ditty.icns</string>
    <key>LSMinimumSystemVersion</key>
    <string>11.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSSupportsAutomaticGraphicsSwitching</key>
    <true/>
</dict>
</plist>
PLIST

# Generate .icns from SVG if tools are available, else skip
if command -v rsvg-convert >/dev/null 2>&1 && command -v iconutil >/dev/null 2>&1; then
	echo "==> Generating app icon"
	ICONSET="$APP_DIR/Contents/Resources/icon.iconset"
	mkdir -p "$ICONSET"
	for size in 16 32 64 128 256 512; do
		rsvg-convert -w "$size" -h "$size" \
			"$SRC_DIR/icons/hicolor/scalable/apps/ditty.svg" \
			-o "$ICONSET/icon_${size}x${size}.png"
	done
	iconutil -c icns "$ICONSET" -o "$APP_DIR/Contents/Resources/ditty.icns"
	rm -rf "$ICONSET"
else
	echo "  WARNING: rsvg-convert or iconutil not found — skipping icon"
fi

# --- PkgInfo ---------------------------------------------------------------
echo "APPL????" >"$APP_DIR/Contents/PkgInfo"

# --- Launcher wrapper ------------------------------------------------------
# The executable in Contents/MacOS/ditty is a wrapper script that sets
# DITTY_LISP_PATH so the REPL finds lisp-fmt.lisp/tui-events.lisp, then
# launches the real binary.
cat >"$APP_DIR/Contents/MacOS/ditty" <<WRAPPER
#!/bin/bash
RES_DIR="\$(dirname "\$0")/../Resources"
export DITTY_LISP_PATH="\$RES_DIR/share/ditty/lisp"
exec "\$(dirname "\$0")/ditty.real" "\$@"
WRAPPER
chmod 755 "$APP_DIR/Contents/MacOS/ditty"

# --- README ----------------------------------------------------------------
echo "==> Writing README-PORTABLE.txt"
cat >"$STAGE_DIR/README-PORTABLE.txt" <<'README'
Ditty (macOS Portable)
======================

This is a self-contained .app bundle of ditty.

Quick start:
  1. Extract the ZIP.
  2. Drag Ditty.app to /Applications (or anywhere).
  3. Double-click Ditty.app, or run from Terminal:
       /path/to/Ditty.app/Contents/MacOS/ditty

The bundle includes all required dylibs and Lisp libraries. No Homebrew
or external dependencies needed.

flare is a syntax-highlighting CLI (cat-like):
  /path/to/Ditty.app/Contents/MacOS/flare somefile.lisp

Emacs integration:
  Add share/emacs/site-lisp to your load-path, then (require 'ditty-mode).

To uninstall:
  Delete Ditty.app. Ditty stores no data outside its own directory.
README

# --- Create ZIP ------------------------------------------------------------
echo "==> Creating ZIP: $(basename "$ZIP_FILE")"
rm -f "$ZIP_FILE"
(cd "$SRC_DIR" && zip -r -9 "$(basename "$ZIP_FILE")" "$PKG_NAME" >/dev/null)

# Clean up staging dir
rm -rf "$STAGE_DIR"

echo "==> Done: $ZIP_FILE"
echo "    Size: $(du -h "$ZIP_FILE" | cut -f1)"
