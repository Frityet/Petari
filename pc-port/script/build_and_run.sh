#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PROJECT_DIR="$ROOT_DIR/pc-port"
TARGET=smg-pc-showcase
SCENE=title
DISC="${SMGPC_DISC_IMAGE:-}"
BUILD_ONLY=false
DEBUG=false
VERIFY=false
LOGS=false
TELEMETRY=false
RUNTIME_ARGS=()

usage() {
    cat <<'USAGE'
Usage: ./pc-port/script/build_and_run.sh [title|gateway|gateway-spin] [options]

Build with Homebrew LLVM 23 for macOS arm64, then launch the selected showcase.
The default is title. One .rvz in the repository root is discovered automatically.

  --disc PATH       Use a particular disc image (or set SMGPC_DISC_IMAGE).
  --build-only      Compile and stage the app bundle without opening a window.
  --strict          Select the unfinished full smg-pc application instead.
  --debug           Launch under LLDB after building.
  --verify          Run the title/gateway smoke check (360-frame limit by default).
  --logs            Also save build/runtime output to pc-port/build/macos-run.log.
  --telemetry       Also stream this process's macOS unified logs.
  --help            Show this help.

Other arguments are passed to the executable, for example --width 1280,
--height 720, --max-frames 600, or --screenshot /tmp/galaxy.png.
Relative runtime output paths are resolved from pc-port.
USAGE
}

fail() { printf '%s\n' "$*" >&2; exit 1; }

while (($#)); do
    case "$1" in
        title|gateway|gateway-spin) SCENE="$1"; shift ;;
        --disc)
            (($# >= 2)) || fail '--disc requires a path'
            DISC="$2"; shift 2 ;;
        --disc=*) DISC="${1#--disc=}"; shift ;;
        --build-only) BUILD_ONLY=true; shift ;;
        --strict) TARGET=smg-pc; shift ;;
        --debug) DEBUG=true; shift ;;
        --verify) VERIFY=true; shift ;;
        --logs) LOGS=true; shift ;;
        --telemetry) TELEMETRY=true; shift ;;
        --help|-h) usage; exit 0 ;;
        --width|--height|--max-frames|--screenshot|--screenshot-frame)
            (($# >= 2)) || fail "$1 requires a value"
            RUNTIME_ARGS+=("$1" "$2"); shift 2 ;;
        --) shift; RUNTIME_ARGS+=("$@"); break ;;
        *) RUNTIME_ARGS+=("$1"); shift ;;
    esac
done

[[ "$(uname -s)" == Darwin && "$(uname -m)" == arm64 ]] ||
    fail 'This launcher requires an Apple Silicon Mac and an arm64 shell.'
command -v brew >/dev/null || fail 'Install Homebrew, then run: brew install llvm@23 xmake cmake ninja'
command -v xmake >/dev/null || fail 'Install Xmake with: brew install xmake'
LLVM_PREFIX="$(brew --prefix llvm@23 2>/dev/null || brew --prefix llvm)"
[[ -x "$LLVM_PREFIX/bin/clang++" ]] || fail 'Install LLVM 23 with: brew install llvm@23'
LLVM_VERSION="$("$LLVM_PREFIX/bin/clang++" --version)"
[[ "$LLVM_VERSION" == *'clang version 23.'* ]] ||
    fail "Expected Homebrew LLVM 23 at $LLVM_PREFIX; found: $LLVM_VERSION"
export PATH="$LLVM_PREFIX/bin:$PATH"

if $VERIFY; then
    [[ "$TARGET" == smg-pc-showcase && "$SCENE" != gateway-spin ]] ||
        fail '--verify supports the title and gateway showcases.'
    ! $DEBUG || fail 'Choose either --debug or --verify.'
    RUNTIME_ARGS+=(--smoke)
fi
if $DEBUG && $TELEMETRY; then
    fail 'Use --telemetry with a normal launch, or --debug for an LLDB session.'
fi

if ! $BUILD_ONLY; then
    if [[ -z "$DISC" ]]; then
        shopt -s nullglob nocaseglob
        DISC_IMAGES=("$ROOT_DIR"/*.rvz)
        shopt -u nullglob nocaseglob
        ((${#DISC_IMAGES[@]} == 1)) ||
            fail 'Pass --disc PATH or put exactly one .rvz in the repository root.'
        DISC="${DISC_IMAGES[0]}"
    fi
    [[ -f "$DISC" ]] || fail "Disc image not found: $DISC"
    # Resolve before changing to the build directory so callers can use relative paths.
    DISC="$(cd "$(dirname "$DISC")" && pwd)/$(basename "$DISC")"
fi

cd "$PROJECT_DIR"
[[ -f aurora/xmake.lua && -f dolphin/CMakeLists.txt ]] ||
    fail 'Initialize dependencies first: git submodule update --init --recursive'
mkdir -p build
if $LOGS; then
    printf 'Saving output to %s/build/macos-run.log\n' "$PROJECT_DIR"
    exec > >(tee "$PROJECT_DIR/build/macos-run.log") 2>&1
fi

BUILD_BINARY="$PROJECT_DIR/build/macosx/arm64/debug/$TARGET"
APP_BUNDLE="$PROJECT_DIR/build/$TARGET.app"
APP_CONTENTS="$APP_BUNDLE/Contents"
BINARY="$APP_CONTENTS/MacOS/$TARGET"
if [[ "$TARGET" == smg-pc-showcase ]]; then
    APP_BUNDLE_ID=org.petari.smg-pc.showcase
    APP_NAME='Super Mario Galaxy Showcase'
else
    APP_BUNDLE_ID=org.petari.smg-pc.game
    APP_NAME='Super Mario Galaxy PC'
fi
PID_FILE="$PROJECT_DIR/build/macos-$TARGET.pid"

# Only stop a process started by this launcher whose executable still matches.
# A separately started game, debugger, or unrelated application is left alone.
if ! $BUILD_ONLY && [[ -f "$PID_FILE" ]]; then
    OLD_PID="$(cat "$PID_FILE")"
    if [[ "$OLD_PID" =~ ^[0-9]+$ ]] &&
       [[ "$(ps -p "$OLD_PID" -o comm= 2>/dev/null || true)" == "$BINARY" ]]; then
        kill -TERM "$OLD_PID"
        for ((attempt = 0; attempt < 50; ++attempt)); do
            kill -0 "$OLD_PID" 2>/dev/null || break
            sleep 0.1
        done
        ! kill -0 "$OLD_PID" 2>/dev/null ||
            fail "Previous launcher process $OLD_PID has not exited; close it before rebuilding."
    fi
    rm -f "$PID_FILE"
fi

xmake f -y -p macosx -a arm64 -m debug -o build --toolchain=llvm \
    --sdk="$LLVM_PREFIX" --ar="$LLVM_PREFIX/bin/llvm-ar" \
    --runtimes=c++_shared --target_minver=26.0
xmake build "$TARGET"
[[ -x "$BUILD_BINARY" ]] || fail "Xmake did not produce the expected executable: $BUILD_BINARY"

# Give SDL/AppKit a native application identity while retaining direct child
# process tracking and command-line arguments. Disc discovery stays in this script.
mkdir -p "$APP_CONTENTS/MacOS"
cp "$BUILD_BINARY" "$BINARY.new"
chmod +x "$BINARY.new"
mv -f "$BINARY.new" "$BINARY"
cat > "$APP_CONTENTS/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleInfoDictionaryVersion</key><string>6.0</string>
    <key>CFBundleExecutable</key><string>$TARGET</string>
    <key>CFBundleIdentifier</key><string>$APP_BUNDLE_ID</string>
    <key>CFBundleName</key><string>$APP_NAME</string>
    <key>CFBundleDisplayName</key><string>$APP_NAME</string>
    <key>CFBundlePackageType</key><string>APPL</string>
    <key>CFBundleShortVersionString</key><string>0.1.0</string>
    <key>CFBundleVersion</key><string>1</string>
    <key>LSMinimumSystemVersion</key><string>26.0</string>
    <key>NSPrincipalClass</key><string>NSApplication</string>
    <key>NSHighResolutionCapable</key><true/>
</dict>
</plist>
PLIST
printf 'Staged app bundle: %s\n' "$APP_BUNDLE"
$BUILD_ONLY && exit 0

APP_ARGS=()
if [[ "$TARGET" == smg-pc-showcase ]]; then
    APP_ARGS+=("$SCENE")
fi
APP_ARGS+=(--disc "$DISC")
# The empty-array guard also supports macOS's system Bash 3.2 with nounset.
if ((${#RUNTIME_ARGS[@]})); then
    APP_ARGS+=("${RUNTIME_ARGS[@]}")
fi
printf 'Launching %s\n' "$BINARY"
if $DEBUG; then
    exec xcrun lldb -- "$BINARY" "${APP_ARGS[@]}"
fi

"$BINARY" "${APP_ARGS[@]}" &
APP_PID=$!
printf '%s\n' "$APP_PID" > "$PID_FILE"
TELEMETRY_PID=
if $TELEMETRY; then
    /usr/bin/log stream --style compact --level info \
        --predicate "processIdentifier == $APP_PID" &
    TELEMETRY_PID=$!
fi
cleanup() {
    if [[ -n "$TELEMETRY_PID" ]]; then
        kill -TERM "$TELEMETRY_PID" 2>/dev/null || true
    fi
    if [[ -f "$PID_FILE" && "$(cat "$PID_FILE")" == "$APP_PID" ]]; then
        rm -f "$PID_FILE"
    fi
}
trap cleanup EXIT
stop_app() {
    kill -TERM "$APP_PID" 2>/dev/null || true
    wait "$APP_PID" 2>/dev/null || true
    exit 130
}
trap stop_app INT TERM
wait "$APP_PID"
