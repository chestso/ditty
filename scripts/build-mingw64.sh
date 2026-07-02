#!/bin/bash
# build-mingw64.sh - Cross-compile ditty for Windows using mingw64.
#
# Usage:
#   scripts/build-mingw64.sh [--no-debug]
#
# Run from the project root. Requires:
#   - x86_64-w64-mingw32-gcc                      (dnf install mingw64-gcc)
#   - x86_64-w64-mingw32-pkg-config + mingw64-pcre2 (dnf install mingw64-pcre2)
#   - autoreconf (run scripts/build-mingw64.sh after a fresh checkout works
#     because we call ../autogen.sh ourselves)
#
# Boehm GC is downloaded and cross-built into deps/gc-mingw64-prefix/ on
# first run, then cached. Output binary: build-mingw64/cli/ditty.exe
# with required runtime DLLs copied alongside.

set -eu

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_ROOT"

ENABLE_DEBUG=true
for arg in "$@"; do
	case "$arg" in
	--no-debug) ENABLE_DEBUG=false ;;
	--help | -h)
		sed -n '2,16p' "$0" | sed 's/^# \{0,1\}//'
		exit 0
		;;
	*)
		echo "Unknown option: $arg" >&2
		exit 1
		;;
	esac
done

BUILD_DIR="build-mingw64"
HOST="x86_64-w64-mingw32"
CC_BIN="${HOST}-gcc"
PKG_CONFIG_BIN="${HOST}-pkg-config"
SYSROOT="/usr/${HOST}/sys-root/mingw"
DEPS_DIR="deps"
GC_VERSION="8.2.6"
GC_URL="https://github.com/ivmai/bdwgc/releases/download/v${GC_VERSION}/gc-${GC_VERSION}.tar.gz"
GC_DIR="${DEPS_DIR}/gc-${GC_VERSION}"
GC_TARBALL="${DEPS_DIR}/gc-${GC_VERSION}.tar.gz"
AO_VERSION="7.8.2"
AO_URL="https://github.com/ivmai/libatomic_ops/releases/download/v${AO_VERSION}/libatomic_ops-${AO_VERSION}.tar.gz"
AO_TARBALL="${DEPS_DIR}/libatomic_ops-${AO_VERSION}.tar.gz"
PARALLEL_JOBS=$(nproc)

command -v "$CC_BIN" >/dev/null 2>&1 || {
	echo "ERROR: mingw64 cross-compiler not found: $CC_BIN" >&2
	echo "Install with: sudo dnf install mingw64-gcc" >&2
	exit 1
}

missing=()
for pkg in mingw64-pcre2; do
	rpm -q "$pkg" >/dev/null 2>&1 || missing+=("$pkg")
done
if [ ${#missing[@]} -gt 0 ]; then
	echo "ERROR: missing mingw64 packages: ${missing[*]}" >&2
	echo "Install with: sudo dnf install ${missing[*]}" >&2
	exit 1
fi

mkdir -p "$DEPS_DIR"

if [ ! -f "$GC_TARBALL" ]; then
	echo "Downloading Boehm GC ${GC_VERSION}..."
	curl -L -o "$GC_TARBALL" "$GC_URL"
fi
if [ ! -d "$GC_DIR" ]; then
	echo "Extracting Boehm GC..."
	tar -xzf "$GC_TARBALL" -C "$DEPS_DIR"
fi
if [ ! -f "$AO_TARBALL" ]; then
	echo "Downloading libatomic_ops ${AO_VERSION}..."
	curl -L -o "$AO_TARBALL" "$AO_URL"
fi
if [ ! -d "${GC_DIR}/libatomic_ops" ]; then
	echo "Extracting libatomic_ops into GC source tree..."
	tar -xzf "$AO_TARBALL" -C "$DEPS_DIR"
	mv "${DEPS_DIR}/libatomic_ops-${AO_VERSION}" "${GC_DIR}/libatomic_ops"
fi

GC_PREFIX="${PROJECT_ROOT}/${DEPS_DIR}/gc-mingw64-prefix"
if [ ! -f "${GC_PREFIX}/lib/libgc.a" ]; then
	echo "Cross-compiling Boehm GC for mingw64..."
	GC_BUILD="${GC_DIR}/build-mingw64"
	mkdir -p "$GC_BUILD"
	(
		cd "$GC_DIR"
		[ -f configure ] || autoreconf -fvi
	)
	(
		cd "${PROJECT_ROOT}/${GC_BUILD}"
		"${PROJECT_ROOT}/${GC_DIR}/configure" \
			--host="${HOST}" \
			--prefix="${GC_PREFIX}" \
			--disable-shared \
			--enable-static \
			--enable-threads=win32 \
			--enable-munmap \
			CFLAGS="-O2"
		make -j"$PARALLEL_JOBS"
		make install
	)
	mkdir -p "${GC_PREFIX}/lib/pkgconfig"
	cat >"${GC_PREFIX}/lib/pkgconfig/bdw-gc.pc" <<-GCPC
		prefix=${GC_PREFIX}
		libdir=\${prefix}/lib
		includedir=\${prefix}/include

		Name: Boehm-Demers-Weiser Conservative Garbage Collector
		Description: A garbage collector for C and C++
		Version: ${GC_VERSION}
		Libs: -L\${libdir} -lgc
		Cflags: -I\${includedir}
	GCPC
	echo "Boehm GC cross-compiled: ${GC_PREFIX}/lib/libgc.a"
else
	echo "Using cached Boehm GC: ${GC_PREFIX}/lib/libgc.a"
fi

# --- Cross-compile boba from sibling repo ---
BOBA_SRC="${PROJECT_ROOT}/../boba"
BOBA_PREFIX="${PROJECT_ROOT}/${DEPS_DIR}/boba-mingw64-prefix"
if [ -d "$BOBA_SRC" ]; then
	if [ ! -f "${BOBA_PREFIX}/lib/libboba.a" ]; then
		echo "Cross-compiling boba for mingw64..."
		BOBA_BUILD="${BOBA_SRC}/build-mingw64"
		rm -rf "$BOBA_BUILD"
		mkdir -p "$BOBA_BUILD"
		(
			cd "$BOBA_SRC"
			[ -f configure ] || ./autogen.sh
		)
		(
			cd "$BOBA_BUILD"
			"${BOBA_SRC}/configure" \
				--host="${HOST}" \
				--prefix="${BOBA_PREFIX}" \
				--disable-shared \
				--enable-static \
				CFLAGS="-O2"
			make -j"$PARALLEL_JOBS"
			make install
		)
		echo "boba cross-compiled: ${BOBA_PREFIX}/lib/libboba.a"
	else
		echo "Using cached boba: ${BOBA_PREFIX}/lib/libboba.a"
	fi
else
	echo "WARN: boba source not found at ${BOBA_SRC}, building without REPL" >&2
	BOBA_PREFIX=""
fi

./autogen.sh

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

CFLAGS_OPT="-O2"
[ "$ENABLE_DEBUG" = true ] && CFLAGS_OPT="-O0 -g3 -DDEBUG"

(
	cd "$BUILD_DIR"
	BOBA_PKG_FLAGS=""
	if [ -n "$BOBA_PREFIX" ] && [ -f "${BOBA_PREFIX}/lib/pkgconfig/boba.pc" ]; then
		BOBA_PKG_FLAGS="PKG_CONFIG_PATH=${BOBA_PREFIX}/lib/pkgconfig:${GC_PREFIX}/lib/pkgconfig"
	fi
	../configure \
		--host="${HOST}" \
		--prefix="${SYSROOT}" \
		PKG_CONFIG="${PKG_CONFIG_BIN}" \
		GC_CFLAGS="-I${GC_PREFIX}/include" \
		GC_LIBS="-L${GC_PREFIX}/lib -lgc" \
		CFLAGS="$CFLAGS_OPT" \
		$BOBA_PKG_FLAGS
	make -j"$PARALLEL_JOBS"
)

EXE_DIR="${BUILD_DIR}/cli"
echo "Collecting runtime DLLs..."
for dll in libpcre2-8-0.dll libgcc_s_seh-1.dll libwinpthread-1.dll; do
	if [ -f "${SYSROOT}/bin/${dll}" ]; then
		cp "${SYSROOT}/bin/${dll}" "${EXE_DIR}/"
	else
		echo "WARN: DLL not found: ${SYSROOT}/bin/${dll}" >&2
	fi
done

echo "Cross-compilation complete: ${EXE_DIR}/ditty.exe"
