#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
REG_DIR="${ROOT_DIR}/third_party/Lama/regression"

rm -f "${REG_DIR}"/*.bc
rm -f "${REG_DIR}/build_examples.sh"
