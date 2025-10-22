#!/usr/bin/env bash
set -euo pipefail

# compile.sh — Build a versioned, reproducible ESP32 firmware using Arduino CLI.
#
# Usage:
#   ./compile.sh -t <c3|c6|s3> [-p <serial-port>] [-l <libs-dir>]
#
# Notes:
# - Serial port is accepted to keep a uniform interface, but not required by compile.
# - Produces: build/builds/<ts>-<ver>-<chip>-<project>/{src,lib,version.txt,output,binary/...}
# - Maintains build cache at build/cache/ to speed up rebuilds.
# - Tracks version in build/.version_state (plain KEY=VALUE file).
# - Increments patch (000 → 001 → …) only on successful compile.
#
# Requirements:
#   - arduino-cli installed & esp32:esp32 core installed
#   - For fallback merge (if *.merged.bin not created), Python esptool is required.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
PROJECT_NAME="${PROJECT_NAME:-$(basename "${PROJECT_ROOT}")}"

# Args
ESP_CHIP=""
ESP_PORT=""  # not used, accepted for interface parity
LIBS_DIR=""

usage() {
  cat <<'EOF'
Usage: compile.sh -t <c3|c6|s3> [-p <serial-port>] [-l <libs-dir>]

Environment:
  PROJECT_NAME       Override project name (default: repo folder name)
  FQBN_EXTRA_OPTS    Comma-separated Arduino menu opts to append (advanced)

EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -t|--type) ESP_CHIP="${2:-}"; shift 2 ;;
    -p|--port) ESP_PORT="${2:-}"; shift 2 ;; # ignored here
    -l|--libs) LIBS_DIR="${2:-}"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown arg: $1"; usage; exit 1 ;;
  esac
done

[[ -z "${ESP_CHIP}" ]] && { echo "❌ Missing -t|--type (c3|c6|s3)"; exit 1; }
[[ "${ESP_CHIP}" =~ ^(c3|c6|s3)$ ]] || { echo "❌ Invalid chip type: ${ESP_CHIP}"; exit 1; }

# Paths
BUILD_ROOT="${PROJECT_ROOT}/build"
WORK_DIR="${BUILD_ROOT}/cache"          # temp build path for Arduino
BUILDS_DIR="${BUILD_ROOT}/builds"

mkdir -p "${WORK_DIR}" "${BUILDS_DIR}"

# Tooling checks
if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "❌ 'arduino-cli' not in PATH. Install: https://arduino.github.io/arduino-cli/latest/installation/"
  exit 1
fi
if ! arduino-cli core list | grep -q 'esp32:esp32'; then
  echo "❌ Espressif core not installed. Run: arduino-cli core install esp32:esp32"
  exit 1
fi

# Chip mapping
chip_to_fqbn_board() {
  case "$1" in
    c3) echo "esp32c3" ;;
    c6) echo "esp32c6" ;;
    s3) echo "esp32s3" ;;
    *)  echo "esp32c3" ;;
  esac
}
chip_to_family_str() {
  case "$1" in
    c3) echo "ESP32-C3" ;;
    c6) echo "ESP32-C6" ;;
    s3) echo "ESP32-S3" ;;
  esac
}
chip_to_esptool_id() {
  case "$1" in
    c3) echo "esp32c3" ;;
    c6) echo "esp32c6" ;;
    s3) echo "esp32s3" ;;
  esac
}

FQBN_BOARD="$(chip_to_fqbn_board "${ESP_CHIP}")"
FQBN_BASE="esp32:esp32:${FQBN_BOARD}"

# Keep FQBN opts minimal & broadly compatible; allow advanced overrides via env.
FQBN_OPTS_DEFAULT="PartitionScheme=no_ota,UploadSpeed=921600"
FQBN_OPTS="${FQBN_OPTS_DEFAULT}"
if [[ -n "${FQBN_EXTRA_OPTS:-}" ]]; then
  FQBN_OPTS="${FQBN_OPTS},${FQBN_EXTRA_OPTS}"
fi
FQBN="${FQBN_BASE}:${FQBN_OPTS}"

# Detect sketch
detect_sketch() {
  local candidate
  # Prefer <project>.ino at repo root
  if [[ -f "${PROJECT_ROOT}/${PROJECT_NAME}.ino" ]]; then
    echo "${PROJECT_ROOT}/${PROJECT_NAME}.ino"; return
  fi
  # First *.ino at repo root
  candidate="$(find "${PROJECT_ROOT}" -maxdepth 1 -name '*.ino' | head -n 1 || true)"
  if [[ -n "${candidate}" ]]; then echo "${candidate}"; return; fi
  # Fallback: src/*.ino
  candidate="$(find "${PROJECT_ROOT}/src" -maxdepth 1 -name '*.ino' 2>/dev/null | head -n 1 || true)"
  if [[ -n "${candidate}" ]]; then echo "${candidate}"; return; fi
  echo ""
}
SKETCH_PATH="$(detect_sketch)"
[[ -n "${SKETCH_PATH}" ]] || { echo "❌ Could not locate a .ino sketch in project root or src/"; exit 1; }

SKETCH_NAME="$(basename "${SKETCH_PATH}" .ino)"

# Version state (INI-like)
STATE_FILE="${BUILD_ROOT}/.version_state"
init_state_if_missing() {
  if [[ ! -f "${STATE_FILE}" ]]; then
    cat > "${STATE_FILE}" <<EOF
MAJOR=0
MINOR=0
PATCH=0
BUILD_ID=0
LAST_BUILD_TS=
PROJECT=${PROJECT_NAME}
EOF
  fi
}
read_kv() { grep -E "^$1=" "${STATE_FILE}" | cut -d'=' -f2- || true; }
write_kv() {
  local k="$1" v="$2"
  if grep -qE "^${k}=" "${STATE_FILE}"; then
    sed -i.bak -E "s|^${k}=.*|${k}=${v}|" "${STATE_FILE}" && rm -f "${STATE_FILE}.bak"
  else
    echo "${k}=${v}" >> "${STATE_FILE}"
  fi
}

init_state_if_missing
MAJOR="$(read_kv MAJOR)"; MINOR="$(read_kv MINOR)"; PATCH="$(read_kv PATCH)"; BUILD_ID="$(read_kv BUILD_ID)"
PATCH_NEXT=$(( 10#$PATCH + 1 ))                    # handle leading zeros safely
BUILD_ID_NEXT=$(( 10#$BUILD_ID + 1 ))
VERSION_NEXT="$(printf "%d.%d.%03d" "${MAJOR}" "${MINOR}" "${PATCH_NEXT}")"

TS="$(date +"%Y%m%d-%H%M%S")"
CHIP_FAMILY="$(chip_to_family_str "${ESP_CHIP}")"
TARGET_DIR="${BUILDS_DIR}/${TS}-${VERSION_NEXT}-${CHIP_FAMILY}-${PROJECT_NAME}"
OUTPUT_DIR="${TARGET_DIR}/output"
BINARY_DIR="${TARGET_DIR}/binary"

echo "🔧 Arduino FQBN: ${FQBN}"
echo "📄 Sketch: ${SKETCH_PATH}"
echo "📦 Using cache: ${WORK_DIR}"
echo "🎯 Build (staging): ${WORK_DIR}"
echo "📁 Build (final):   ${TARGET_DIR}"

rm -rf "${WORK_DIR}"
mkdir -p "${WORK_DIR}" "${OUTPUT_DIR}" "${BINARY_DIR}"

# Build arguments
COMPILE_ARGS=(
  compile
  --fqbn "${FQBN}"
  --build-path "${WORK_DIR}"
  --warnings default
)

# Libraries: prefer user-supplied; also accept project’s own lib/ if exists
declare -a LIB_FLAGS=()
if [[ -n "${LIBS_DIR}" ]]; then
  if [[ -d "${LIBS_DIR}" ]]; then
    LIB_FLAGS+=( --libraries "${LIBS_DIR}" )
    echo "📚 Using libs from: ${LIBS_DIR}"
  else
    echo "⚠️  Provided libs path doesn't exist: ${LIBS_DIR}"
  fi
fi
if [[ -d "${PROJECT_ROOT}/lib" ]]; then
  LIB_FLAGS+=( --libraries "${PROJECT_ROOT}/lib" )
  echo "📚 Also using project lib/: ${PROJECT_ROOT}/lib"
fi

# Run compile
set +e
if ((${#LIB_FLAGS[@]})); then
  arduino-cli "${COMPILE_ARGS[@]}" "${LIB_FLAGS[@]}" "${SKETCH_PATH}" | tee "${WORK_DIR}/compile.log"
else
  arduino-cli "${COMPILE_ARGS[@]}" "${SKETCH_PATH}" | tee "${WORK_DIR}/compile.log"
fi
BUILD_RC="${PIPESTATUS[0]}"
set -e

if [[ "${BUILD_RC}" -ne 0 ]]; then
  echo "❌ Compile failed (exit ${BUILD_RC}). See ${WORK_DIR}/compile.log"
  exit "${BUILD_RC}"
fi

# Find or create merged binary
MERGED_BIN="$(find "${WORK_DIR}" -maxdepth 1 -name "${SKETCH_NAME}.ino.merged.bin" -print -quit || true)"
if [[ -z "${MERGED_BIN}" ]]; then
  echo "ℹ️  No *.merged.bin found. Attempting manual merge via esptool…"
  if ! python - <<'PYCHK' >/dev/null 2>&1
import importlib.util, sys
sys.exit(0 if importlib.util.find_spec("esptool") else 1)
PYCHK
  then
    echo "❌ esptool (Python) not found; cannot merge binaries."
    echo "   Install with: pip install esptool"
    exit 1
  fi

  APP_BIN="$(find "${WORK_DIR}" -maxdepth 1 -name "${SKETCH_NAME}.ino.bin" -print -quit || true)"
  BOOT_BIN="$(find "${WORK_DIR}" -type f -name 'bootloader*.bin' -print -quit || true)"
  PART_BIN="$(find "${WORK_DIR}" -type f -name '*partitions*.bin' -print -quit || true)"

  [[ -f "${APP_BIN}" && -f "${BOOT_BIN}" && -f "${PART_BIN}" ]] || {
    echo "❌ Missing components for merge (bootloader/partitions/app)."
    exit 1
  }

  # Common ESP32 layout: boot=0x0, partitions=0x8000, app=0x10000
  MERGED_BIN="${WORK_DIR}/${SKETCH_NAME}.ino.merged.bin"
  python -m esptool merge_bin -o "${MERGED_BIN}" \
    0x0 "${BOOT_BIN}" \
    0x8000 "${PART_BIN}" \
    0x10000 "${APP_BIN}"
  echo "✅ Created merged image: ${MERGED_BIN}"
else
  echo "✅ Found merged image: ${MERGED_BIN}"
fi

# Snapshot project sources
if [[ -d "${PROJECT_ROOT}/src" ]]; then
  cp -a "${PROJECT_ROOT}/src" "${TARGET_DIR}/src"
else
  mkdir -p "${TARGET_DIR}/src"
  cp -a "${SKETCH_PATH}" "${TARGET_DIR}/src/"
fi

# Snapshot libraries if provided
if [[ -n "${LIBS_DIR}" && -d "${LIBS_DIR}" ]]; then
  cp -a "${LIBS_DIR}" "${TARGET_DIR}/lib"
fi

# Version file
echo "${VERSION_NEXT}" > "${TARGET_DIR}/version.txt"

# Copy build outputs & binaries
cp -a "${WORK_DIR}/." "${OUTPUT_DIR}/"
cp -a "${MERGED_BIN}" "${BINARY_DIR}/firmware.bin"
cp -a "${MERGED_BIN}" "${BINARY_DIR}/${VERSION_NEXT}-${CHIP_FAMILY}-${PROJECT_NAME}.bin"

# Manifest (ESP Web Tools v10)
cat > "${BINARY_DIR}/manifest.json" <<EOF
{
  "name": "${PROJECT_NAME}",
  "version": "latest",
  "new_install_improv_wait_time": 0,
  "builds": [
    {
      "chipFamily": "${CHIP_FAMILY}",
      "parts": [
        { "path": "firmware.bin", "offset": 0 }
      ]
    }
  ]
}
EOF
echo "📝 Wrote manifest → ${BINARY_DIR}/manifest.json"

# Update state (commit the new version)
write_kv PATCH "$(printf "%03d" "${PATCH_NEXT}")"
write_kv BUILD_ID "${BUILD_ID_NEXT}"
write_kv LAST_BUILD_TS "${TS}"
write_kv PROJECT "${PROJECT_NAME}"

# Maintain 'latest' symlink for convenience
ln -sfn "${TARGET_DIR}" "${BUILD_ROOT}/latest"

echo
echo "🎉 Build complete."
echo "   ➤ Final dir : ${TARGET_DIR}"
echo "   ➤ Firmware  : ${BINARY_DIR}/firmware.bin"
echo "   ➤ Version   : ${VERSION_NEXT}"
