#!/usr/bin/env bash
set -euo pipefail

# build.sh — Orchestrates compile → upload → serial monitor for ESP32.
#
# Usage:
#   ./build.sh -t <c3|c6|s3> -p <serial-port> [-l <libs-dir>] [-b <baud>] [--compile-only] [--no-monitor]
#
# Examples:
#   ./build.sh -t c3 -p /dev/ttyUSB0
#   ./build.sh -t s3 -p /dev/cu.usbmodem1101 -l ../lib --no-monitor
#
# Notes:
# - All activity stays inside ./build/
# - Exports env vars so the sub-scripts share inputs.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

usage() {
  cat <<'EOF'
Usage: build.sh -t <c3|c6|s3> -p <serial-port> [-l <libs-dir>] [-b <baud>]
                [--compile-only] [--no-upload] [--no-monitor]

Options:
  -t, --type         ESP32 chip type: c3 | c6 | s3   (required)
  -p, --port         Serial port, e.g. /dev/ttyUSB0  (required, for upload/monitor)
  -l, --libs         Path to libraries folder (optional)
  -b, --baud         Serial/upload baud rate (default: 921600)
      --compile-only Only compile (skip upload and monitor)
      --no-upload    Compile only (skip upload)
      --no-monitor   Don't open serial monitor after upload
  -h, --help         Show this help

Environment overrides:
  PROJECT_NAME       Overrides auto-detected project name (default: repo folder name)
  FQBN_EXTRA_OPTS    Comma-separated Arduino FQBN menu opts to append (advanced)

EOF
}

# Defaults
ESP_CHIP=""
ESP_PORT=""
LIBS_DIR=""
ESP_BAUD="921600"
DO_COMPILE=1
DO_UPLOAD=1
DO_MONITOR=1

# Parse args
ARGS=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    -t|--type) ESP_CHIP="${2:-}"; shift 2 ;;
    -p|--port) ESP_PORT="${2:-}"; shift 2 ;;
    -l|--libs) LIBS_DIR="${2:-}"; shift 2 ;;
    -b|--baud) ESP_BAUD="${2:-}"; shift 2 ;;
    --compile-only) DO_UPLOAD=0; DO_MONITOR=0; shift ;;
    --no-upload) DO_UPLOAD=0; shift ;;
    --no-monitor) DO_MONITOR=0; shift ;;
    -h|--help) usage; exit 0 ;;
    *) ARGS+=("$1"); shift ;;
  esac
done

[[ -z "${ESP_CHIP}" ]] && { echo "❌ Missing -t|--type (c3|c6|s3)"; usage; exit 1; }
[[ "${ESP_CHIP}" =~ ^(c3|c6|s3)$ ]] || { echo "❌ Invalid chip type: ${ESP_CHIP}"; exit 1; }

if [[ "${DO_UPLOAD}" -eq 1 || "${DO_MONITOR}" -eq 1 ]]; then
  [[ -z "${ESP_PORT}" ]] && { echo "❌ Missing -p|--port for upload/monitor"; usage; exit 1; }
fi

export ESP_CHIP ESP_PORT LIBS_DIR ESP_BAUD

# 1) Compile
if [[ "${DO_COMPILE}" -eq 1 ]]; then
  echo "🧱 Step 1/3: Compile"
  ./compile.sh -t "${ESP_CHIP}" ${ESP_PORT:+-p "${ESP_PORT}"} ${LIBS_DIR:+-l "${LIBS_DIR}"} || {
    echo "❌ Compile failed"; exit 1;
  }
fi

# 2) Upload
if [[ "${DO_UPLOAD}" -eq 1 ]]; then
  echo "📤 Step 2/3: Upload"
  ./upload.sh -t "${ESP_CHIP}" -p "${ESP_PORT}" -b "${ESP_BAUD}" || {
    echo "❌ Upload failed"; exit 1;
  }
fi

# 3) Monitor
if [[ "${DO_MONITOR}" -eq 1 ]]; then
  echo "🖥️  Step 3/3: Serial monitor (Ctrl-C to exit)"
  ./listen_serial.sh -p "${ESP_PORT}" -b "${ESP_BAUD}"
fi

echo "✅ Done."
