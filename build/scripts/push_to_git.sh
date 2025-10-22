#!/usr/bin/env bash
set -euo pipefail

# Publish exactly two artifacts to branch "binaries" without touching the
# current branch, working tree, or index. No new files are created in the repo.

PROJECT_ROOT=""
TARGET_DIR=""
VERSION_STR=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --project-root) PROJECT_ROOT="${2:-}"; shift 2 ;;
    --target-dir)   TARGET_DIR="${2:-}"; shift 2 ;;
    --version)      VERSION_STR="${2:-}"; shift 2 ;;
    -h|--help)
      echo "usage: $0 --project-root <abs> --target-dir <abs> --version <x.y.zzz>"
      exit 0 ;;
    *) echo "unknown arg: $1"; exit 2 ;;
  esac
done

[[ -d "${PROJECT_ROOT}" ]] || { echo "project-root invalid"; exit 2; }
[[ -d "${TARGET_DIR}"   ]] || { echo "target-dir invalid"; exit 2; }
[[ -n "${VERSION_STR}"  ]] || { echo "version missing"; exit 2; }

# Locate artifacts.
BIN_FILE="$(find "${TARGET_DIR}/binary" -maxdepth 1 -type f -name "*.bin" | head -n1 || true)"
MANIFEST_FILE="$(find "${TARGET_DIR}" -maxdepth 2 -type f -name "manifest.json" | head -n1 || true)"
[[ -n "${BIN_FILE}" && -f "${MANIFEST_FILE}" ]] || { echo "artifacts missing"; exit 0; }

# Ensure paths are inside repo and capture relative paths.
case "${BIN_FILE}" in
  "${PROJECT_ROOT}/"*) REL_BIN="${BIN_FILE#"${PROJECT_ROOT}/"}" ;;
  *) echo "binary not under repo"; exit 2 ;;
esac
case "${MANIFEST_FILE}" in
  "${PROJECT_ROOT}/"*) REL_MAN="${MANIFEST_FILE#"${PROJECT_ROOT}/"}" ;;
  *) echo "manifest not under repo"; exit 2 ;;
esac

# Get base commit for binaries branch.
git -C "${PROJECT_ROOT}" fetch origin binaries >/dev/null 2>&1 || true
if git -C "${PROJECT_ROOT}" ls-remote --exit-code --heads origin binaries >/dev/null 2>&1; then
  BASE_COMMIT="$(git -C "${PROJECT_ROOT}" rev-parse origin/binaries^{commit})"
else
  git -C "${PROJECT_ROOT}" fetch origin >/dev/null 2>&1 || true
  BASE_COMMIT="$(git -C "${PROJECT_ROOT}" rev-parse HEAD)"
fi
BASE_TREE="$(git -C "${PROJECT_ROOT}" rev-parse "${BASE_COMMIT}^{tree}")"

# Use a separate temporary index. Do not touch user index or worktree.
INDEX_FILE="$(mktemp)"
cleanup() { rm -f "${INDEX_FILE}"; }
trap cleanup EXIT
export GIT_INDEX_FILE="${INDEX_FILE}"

# Start from base tree, then replace only the two paths.
git -C "${PROJECT_ROOT}" read-tree "${BASE_TREE}"

BIN_BLOB="$(git -C "${PROJECT_ROOT}" hash-object -w -- "${BIN_FILE}")"
MAN_BLOB="$(git -C "${PROJECT_ROOT}" hash-object -w -- "${MANIFEST_FILE}")"

git -C "${PROJECT_ROOT}" update-index --add --cacheinfo 100644 "${BIN_BLOB}" "${REL_BIN}"
git -C "${PROJECT_ROOT}" update-index --add --cacheinfo 100644 "${MAN_BLOB}" "${REL_MAN}"

NEW_TREE="$(git -C "${PROJECT_ROOT}" write-tree)"
MSG="Version ${VERSION_STR} is compiled and uploaded succesfully."
NEW_COMMIT="$(git -C "${PROJECT_ROOT}" commit-tree "${NEW_TREE}" -p "${BASE_COMMIT}" -m "${MSG}")"

# Push commit to binaries branch without checking it out.
git -C "${PROJECT_ROOT}" push origin "${NEW_COMMIT}:refs/heads/binaries"
