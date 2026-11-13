#!/usr/bin/env bash
# =====================================================================
# Mcl0v1n vs android - top-level reproducible build
# Builds: native su/daemon (NDK) + manager APK (SDK) + flashable zip.
# Host: Linux/macOS with Android NDK r10e and SDK (API 19).
# =====================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/out"
mkdir -p "$OUT"

: "${NDK_HOME:?Set NDK_HOME to your android-ndk-r10e path}"
: "${ANDROID_SDK:?Set ANDROID_SDK to your android-sdk path (needs API 19)}"

echo "==> [1/4] Building native binaries (su, mclovind, mclovin_sepatch)"
cd "$ROOT/native"
"$NDK_HOME/ndk-build" \
    NDK_PROJECT_PATH="$ROOT/native" \
    APP_BUILD_SCRIPT="$ROOT/native/su/Android.mk" \
    NDK_APPLICATION_MK="$ROOT/native/Application.mk" \
    NDK_OUT="$ROOT/native/obj" \
    NDK_LIBS_OUT="$ROOT/native/libs"
"$NDK_HOME/ndk-build" \
    NDK_PROJECT_PATH="$ROOT/native" \
    APP_BUILD_SCRIPT="$ROOT/native/resetprop/Android.mk" \
    NDK_APPLICATION_MK="$ROOT/native/Application.mk" \
    NDK_OUT="$ROOT/native/obj" \
    NDK_LIBS_OUT="$ROOT/native/libs"

# ndk-build drops executables in obj/local/<abi>/
SU_BIN="$ROOT/native/obj/local/armeabi-v7a/su"
SE_BIN="$ROOT/native/obj/local/armeabi-v7a/mclovin_sepatch"
[ -f "$SU_BIN" ] || { echo "su binary not built"; exit 1; }

echo "==> [2/4] Staging binaries into flashable payload"
cp "$SU_BIN" "$ROOT/flashable/mclovin/bin/su"
cp "$SE_BIN" "$ROOT/flashable/mclovin/bin/mclovin_sepatch"
chmod 0755 "$ROOT/flashable/mclovin/bin/"*

echo "==> [3/4] Building manager APK"
"$ROOT/scripts/build_app.sh"
cp "$ROOT/app/bin/Mcl0v1n-release.apk" "$ROOT/flashable/mclovin/Mcl0v1n.apk" 2>/dev/null \
  || cp "$ROOT/app/bin/Mcl0v1n-debug.apk" "$ROOT/flashable/mclovin/Mcl0v1n.apk"
cp "$ROOT/flashable/mclovin/Mcl0v1n.apk" "$OUT/Mcl0v1n.apk"

echo "==> [4/4] Packaging + signing flashable zip"
"$ROOT/scripts/make_zip.sh"

echo
echo "Build complete:"
echo "  $OUT/Mcl0v1n-vs-android-i9300-signed.zip   (flash in TWRP/CWM)"
echo "  $OUT/Mcl0v1n.apk                            (standalone manager apk)"
