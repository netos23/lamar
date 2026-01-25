#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
REG_DIR="${ROOT_DIR}/third_party/Lama/regression"
EXAMPLES_DIR="${ROOT_DIR}/examples"

USE_DOCKER=false
BUILD_ARGS=""
while getopts "d" opt; do
  case "$opt" in
    d) USE_DOCKER=true; BUILD_ARGS="-d" ;;
    *) echo "Usage: $0 [-d]" >&2; exit 1 ;;
  esac
done
shift $((OPTIND - 1))

mkdir -p "${EXAMPLES_DIR}"
cp "${ROOT_DIR}/scripts/build_examples.sh" "${REG_DIR}/build_examples.sh"
chmod +x "${REG_DIR}/build_examples.sh"

if ${USE_DOCKER}; then
  docker run --mount type=bind,src="${ROOT_DIR}/third_party/Lama",dst=/mnt/Lama -w /mnt/Lama/regression dlama:1.0.0 ./build_examples.sh ${BUILD_ARGS}
else
  (cd "${REG_DIR}" && ./build_examples.sh ${BUILD_ARGS})
fi

shopt -s nullglob
files=("${REG_DIR}"/*.input "${REG_DIR}"/*.lama "${REG_DIR}"/*.t "${REG_DIR}"/*.bc)
if [ ${#files[@]} -gt 0 ]; then
  cp "${files[@]}" "${EXAMPLES_DIR}/"
fi
shopt -u nullglob

"${ROOT_DIR}/scripts/clean_temp.sh"
