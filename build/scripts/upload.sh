#!/usr/bin/env bash
set -euo pipefail

# upload.sh — Flash the latest compiled firmware using esptool.
#
# Usage:
#   ./upload.sh -t <c3|c6|s3> -p <serial-port> [-b <baud>]
#
# Notes:
# - Looks for firmware at build/latest/binary/firmware.bin
# - Requires esptool (either esptool.py in PATH or 'python -m esptool').

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
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

chip_to_esptool_id() {
  case "$1" in
    c3) echo "esp32c3" ;;
    c6) echo "esp32c6" ;;
    s3) echo "esp32s3" ;;
    *)  echo "esp32c3" ;;
  esac
}

ESPID="$(chip_to_esptool_id "${ESP_CHIP}")"
LATEST_DIR="${BUILD_ROOT}/latest"
FW="${LATEST_DIR}/binary/firmware.bin"

[[ -f "${FW}" ]] || { echo "❌ Firmware not found: ${FW} (build first)"; exit 1; }

# Pick esptool runner
if command -v esptool.py >/dev/null 2>&1; then
  ESPTOOL=(esptool.py)
else
  ESPTOOL=(python -m esptool)
fi

echo "🚀 Flashing ${FW}"
echo "    port: ${ESP_PORT}, chip: ${ESPID}, baud: ${ESP_BAUD}"

# Write merged image to 0x0
"${ESPTOOL[@]}" \
  --chip "${ESPID}" \
  --port "${ESP_PORT}" \
  --baud "${ESP_BAUD}" \
  write_flash 0x0 "${FW}"

echo "✅ Flash complete."
