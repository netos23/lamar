#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

DIRS=()
if [ "$#" -gt 0 ]; then
  DIRS=("$@")
else
  DIRS=(
    "${ROOT_DIR}/third_party/Lama/regression"
    "${ROOT_DIR}/third_party/Lama/performance"
  )
fi

for dir in "${DIRS[@]}"; do
  [ -d "${dir}" ] || continue
  rm -f "${dir}"/*.bc
  rm -f "${dir}/build_examples.sh"
done
