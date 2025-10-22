#!/usr/bin/env bash
set -euo pipefail

# upload.sh — Flash the latest compiled firmware.
# 1) Prefer arduino-cli upload using the per-build output dir
# 2) Fallback to esptool (esptool.py / python3 -m esptool / python -m esptool)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_ROOT="${PROJECT_ROOT}/build"

ESP_CHIP=""
ESP_PORT=""
ESP_BAUD="921600"

usage() {
  cat <<'EOF'
Usage: upload.sh -t <c3|c6|s3> -p <serial-port> [-b <baud>]
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -t|--type) ESP_CHIP="${2:-}"; shift 2 ;;
    -p|--port) ESP_PORT="${2:-}"; shift 2 ;;
    -b|--baud) ESP_BAUD="${2:-}"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown arg: $1"; usage; exit 1 ;;
  esac
done

[[ -z "${ESP_CHIP}" ]] && { echo "❌ Missing -t|--type (c3|c6|s3)"; exit 1; }
[[ -z "${ESP_PORT}" ]] && { echo "❌ Missing -p|--port"; exit 1; }

# Resolve 'latest' build (symlink or newest folder)
LATEST_DIR="${BUILD_ROOT}/latest"
if [[ ! -e "${LATEST_DIR}" ]]; then
  LATEST_DIR="$(ls -1dt "${BUILD_ROOT}/builds"/* 2>/dev/null | head -n 1 || true)"
fi
[[ -n "${LATEST_DIR}" ]] || { echo "❌ No builds found. Run compile first."; exit 1; }

OUTPUT_DIR="${LATEST_DIR}/output"
FW="${LATEST_DIR}/binary/firmware.bin"
[[ -d "${OUTPUT_DIR}" ]] || { echo "❌ Output dir not found: ${OUTPUT_DIR}"; exit 1; }
[[ -f "${FW}" ]] || { echo "❌ Firmware not found: ${FW}"; exit 1; }

# Chip helpers
chip_to_fqbn_board() {
  case "$1" in
    c3) echo "esp32c3" ;;
    c6) echo "esp32c6" ;;
    s3) echo "esp32s3" ;;
    *)  echo "esp32c3" ;;
  esac
}
chip_to_esptool_id() {
  case "$1" in
    c3) echo "esp32c3" ;;
    c6) echo "esp32c6" ;;
    s3) echo "esp32s3" ;;
    *)  echo "esp32c3" ;;
  esac
}

BOARD="$(chip_to_fqbn_board "${ESP_CHIP}")"
FQBN_BASE="esp32:esp32:${BOARD}"
FQBN_OPTS_DEFAULT="PartitionScheme=no_ota,UploadSpeed=${ESP_BAUD}"
FQBN_OPTS="${FQBN_OPTS_DEFAULT}${FQBN_EXTRA_OPTS:+,${FQBN_EXTRA_OPTS}}"
FQBN="${FQBN_BASE}:${FQBN_OPTS}"

echo "📤 Uploading from: ${OUTPUT_DIR}"

# 1) Prefer arduino-cli upload (no Python needed)
if command -v arduino-cli >/dev/null 2>&1; then
  echo "🚀 Using arduino-cli upload → ${ESP_PORT} (${FQBN})"
  set +e
  arduino-cli upload \
    --fqbn "${FQBN}" \
    --port "${ESP_PORT}" \
    --input-dir "${OUTPUT_DIR}"
  RC=$?
  set -e
  if [[ $RC -eq 0 ]]; then
    echo "✅ Upload complete via arduino-cli."
    exit 0
  else
    echo "⚠️  arduino-cli upload failed (rc=${RC}); falling back to esptool…"
  fi
fi

# 2) Fallback to esptool on the merged image @ 0x0
ESPID="$(chip_to_esptool_id "${ESP_CHIP}")"

if command -v esptool.py >/dev/null 2>&1; then
  ESPTOOL=(esptool.py)
elif command -v python3 >/dev/null 2>&1; then
  ESPTOOL=(python3 -m esptool)
elif command -v python >/dev/null 2>&1; then
  ESPTOOL=(python -m esptool)
else
  echo "❌ No Python/esptool found."
  echo "   Options:"
  echo "     • Homebrew:  brew install esptool"
  echo "     • or pip3:   pip3 install --user esptool"
  echo "   Then re-run:   ./upload.sh -t ${ESP_CHIP} -p ${ESP_PORT}"
  exit 1
fi

echo "🚀 Flashing ${FW} with ${ESPID} at ${ESP_BAUD} on ${ESP_PORT}"
"${ESPTOOL[@]}" \
  --chip "${ESPID}" \
  --port "${ESP_PORT}" \
  --baud "${ESP_BAUD}" \
  write_flash 0x0 "${FW}"

echo "✅ Upload complete via esptool."
