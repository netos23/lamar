#!/usr/bin/env bash
set -euo pipefail

# Measure runtime of lamar / lamac (-i, -s) for each *.bc file.
# Environment overrides:
#   BC_DIR     - directory to scan for *.bc (default: <repo>/performance)
#   LAMAR_BIN  - path to lamar executable (default: <repo>/cmake-build-release/lamar)
#   LAMAC_BIN  - path to lamac executable or name resolvable via PATH (default: lamac)

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BC_DIR="${BC_DIR:-${ROOT_DIR}/performance}"
LAMAR_BIN="${LAMAR_BIN:-${ROOT_DIR}/cmake-build-release/lamar}"
LAMAC_BIN="${LAMAC_BIN:-lamac}"

if [[ ! -x "${LAMAR_BIN}" ]]; then
  echo "::error file=${LAMAR_BIN}::lamar executable not found or not executable" >&2
  exit 1
fi

if [[ "${LAMAC_BIN}" == */* ]]; then
  if [[ ! -x "${LAMAC_BIN}" ]]; then
    echo "::error file=${LAMAC_BIN}::lamac executable not found or not executable" >&2
    exit 1
  fi
else
  if ! command -v "${LAMAC_BIN}" >/dev/null 2>&1; then
    echo "::error file=${LAMAC_BIN}::lamac executable not found in PATH" >&2
    exit 1
  fi
  LAMAC_BIN="$(command -v "${LAMAC_BIN}")"
fi

if [[ ! -d "${BC_DIR}" ]]; then
  echo "::error file=${BC_DIR}::.bc directory does not exist" >&2
  exit 1
fi

BC_FILES=()
while IFS= read -r bc_file; do
  BC_FILES+=("${bc_file}")
done < <(find "${BC_DIR}" -type f -name '*.bc' | sort)

if [[ ${#BC_FILES[@]} -eq 0 ]]; then
  echo "::warning::No .bc files found under ${BC_DIR}" >&2
  exit 0
fi

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "${TMP_DIR}"' EXIT

RESULTS=()
FAILURES=()
MEASURE_ID=0

measure_time() {
  local label="$1" stdin_src="$2"
  shift 2

  local id=${MEASURE_ID}
  ((++MEASURE_ID))

  local err_file="${TMP_DIR}/err_${id}.log"
  local status=0 duration

  TIMEFORMAT=%R
  if [[ -n "${stdin_src}" ]]; then
    duration="$({ time "$@" <"${stdin_src}" >/dev/null 2>"${err_file}"; } 2>&1)" || status=$?
  else
    duration="$({ time "$@" </dev/null >/dev/null 2>"${err_file}"; } 2>&1)" || status=$?
  fi

  if (( status != 0 )); then
    echo "::error file=${label}::Command failed (exit ${status})" >&2
    FAILURES+=("${label}: exit ${status} (stderr: ${err_file})")
    return 1
  fi

  RESULTS+=("${label}|${duration}")
  return 0
}

echo "Running performance measurements for ${#BC_FILES[@]} bytecode files..." >&2

for BC_FILE in "${BC_FILES[@]}"; do
  DIRNAME="$(dirname "${BC_FILE}")"
  BASENAME="$(basename "${BC_FILE}" .bc)"
  LAMA_FILE="${DIRNAME}/${BASENAME}.lama"
  INPUT_FILE="${DIRNAME}/${BASENAME}.input"
  STDIN_SRC="/dev/null"
  [[ -f "${INPUT_FILE}" ]] && STDIN_SRC="${INPUT_FILE}"

  if [[ ! -f "${LAMA_FILE}" ]]; then
    echo "::error file=${LAMA_FILE}::matching .lama file not found" >&2
    FAILURES+=("${LAMA_FILE}: missing")
    continue
  fi

  measure_time "lamar ${BC_FILE}" "${STDIN_SRC}" "${LAMAR_BIN}" "${BC_FILE}" || true
  measure_time "lamac -i ${LAMA_FILE}" "${STDIN_SRC}" "${LAMAC_BIN}" -i "${LAMA_FILE}" || true
  measure_time "lamac -s ${LAMA_FILE}" "${STDIN_SRC}" "${LAMAC_BIN}" -s "${LAMA_FILE}" || true

done

if [[ ${#RESULTS[@]} -eq 0 ]]; then
  echo "::error::No successful measurements recorded" >&2
  exit 1
fi

SORTED=$(printf '%s\n' "${RESULTS[@]}" | sort -t'|' -k2,2n)

printf "%-50s %12s %12s\n" "Program" "Seconds" "% prev"
PREV_TIME=""
while IFS='|' read -r label duration; do
  [[ -z "${duration}" ]] && continue
  if [[ -z "${PREV_TIME}" ]]; then
    delta="n/a"
  else
    delta=$(awk -v c="${duration}" -v p="${PREV_TIME}" 'BEGIN { if (p == 0) { print "n/a" } else { printf "%.2f%%", ((c - p) / p) * 100 } }')
  fi
  printf "%-50s %12.4f %12s\n" "${label}" "${duration}" "${delta}"
  PREV_TIME="${duration}"
done <<< "${SORTED}"

if (( ${#FAILURES[@]} > 0 )); then
  echo "" >&2
  echo "Warnings/Failures:" >&2
  for ITEM in "${FAILURES[@]}"; do
    echo "- ${ITEM}" >&2
  done
fi
