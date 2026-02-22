#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SOURCE_DIR="${ROOT_DIR}/third_party/Lama/performance"
DEST_DIR="${ROOT_DIR}/performance"

"${ROOT_DIR}/scripts/make_suite.sh" "$@" "${SOURCE_DIR}" "${DEST_DIR}" "*.lama" "*.bc"
