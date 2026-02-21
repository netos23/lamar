#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "Usage: $0 [-d] <source_dir> <destination_dir> <copy_glob>..." >&2
  exit 1
}

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

USE_DOCKER=false
BUILD_ARGS=""
while getopts "d" opt; do
  case "$opt" in
    d) USE_DOCKER=true; BUILD_ARGS="-d" ;;
    *) usage ;;
  esac
done
shift $((OPTIND - 1))

if [ "$#" -lt 3 ]; then
  usage
fi

SOURCE_DIR="$1"
DEST_DIR="$2"
shift 2
COPY_PATTERNS=("$@")

if [ ! -d "${SOURCE_DIR}" ]; then
  echo "Source directory not found: ${SOURCE_DIR}" >&2
  exit 1
fi

mkdir -p "${DEST_DIR}"

cp "${ROOT_DIR}/scripts/build_examples.sh" "${SOURCE_DIR}/build_examples.sh"
chmod +x "${SOURCE_DIR}/build_examples.sh"

SUITE_PARENT="$(cd "$(dirname "${SOURCE_DIR}")" && pwd)"
SUITE_NAME="$(basename "${SOURCE_DIR}")"

if ${USE_DOCKER}; then
  docker run --mount type=bind,src="${SUITE_PARENT}",dst=/mnt/Lama -w "/mnt/Lama/${SUITE_NAME}" dlama:1.0.0 ./build_examples.sh ${BUILD_ARGS}
else
  (cd "${SOURCE_DIR}" && ./build_examples.sh ${BUILD_ARGS})
fi

shopt -s nullglob
files=()
for pattern in "${COPY_PATTERNS[@]}"; do
  files+=("${SOURCE_DIR}"/${pattern})
done
if [ ${#files[@]} -gt 0 ]; then
  cp "${files[@]}" "${DEST_DIR}/"
fi
shopt -u nullglob

"${ROOT_DIR}/scripts/clean_temp.sh" "${SOURCE_DIR}"
