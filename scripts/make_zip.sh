#!/usr/bin/env bash
# =====================================================================
# Mcl0v1n vs android - assemble + sign the recovery-flashable zip.
# =====================================================================
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/out"
mkdir -p "$OUT"

STAGE="$OUT/zip-stage"
rm -rf "$STAGE"; mkdir -p "$STAGE"

# Copy flashable tree (installer + payload)
cp -r "$ROOT/flashable/META-INF" "$STAGE/"
cp -r "$ROOT/flashable/mclovin"  "$STAGE/"

# update-binary must be executable inside the zip
chmod 0755 "$STAGE/META-INF/com/google/android/update-binary"

UNSIGNED="$OUT/Mcl0v1n-vs-android-i9300-unsigned.zip"
SIGNED="$OUT/Mcl0v1n-vs-android-i9300-signed.zip"

( cd "$STAGE" && zip -r -X "$UNSIGNED" . >/dev/null )

# Sign the zip so recoveries with signature verification accept it.
# zipsigner.jar (AOSP SignApk) is the classic tool; fall back to leaving it
# unsigned (TWRP can disable verification).
if [ -f "$ROOT/prebuilt/zipsigner.jar" ]; then
  echo "==> Signing zip with zipsigner.jar"
  java -jar "$ROOT/prebuilt/zipsigner.jar" "$UNSIGNED" "$SIGNED"
else
  echo "==> zipsigner.jar not found; shipping unsigned (disable zip verify in TWRP)"
  cp "$UNSIGNED" "$SIGNED"
fi

echo "Flashable zip: $SIGNED"
