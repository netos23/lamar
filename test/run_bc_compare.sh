#!/usr/bin/env bash
set -euo pipefail

# Compare byterun and lamar outputs for all .bc files.
# Environment overrides:
#   BC_DIR       - directory to scan for *.bc (default: <repo>/examples)
#   BYTERUN_BIN  - path to byterun executable
#   LAMAR_BIN    - path to lamar executable

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BC_DIR="${BC_DIR:-${ROOT_DIR}/examples}"
BYTERUN_BIN="${BYTERUN_BIN:-${ROOT_DIR}/cmake-build-debug/third_party/Lama/byterun/byterun}"
LAMAR_BIN="${LAMAR_BIN:-${ROOT_DIR}/cmake-build-debug/lamar}"

if [[ ! -x "${BYTERUN_BIN}" ]]; then
  echo "::error file=${BYTERUN_BIN}::byterun executable not found or not executable" >&2
  exit 1
fi

if [[ ! -x "${LAMAR_BIN}" ]]; then
  echo "::error file=${LAMAR_BIN}::lamar executable not found or not executable" >&2
  exit 1
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

TOTAL=0
PASS=0
FAIL=0
FAILURES=()

echo "Running ${#BC_FILES[@]} bytecode checks..."

for BC_FILE in "${BC_FILES[@]}"; do
  ((TOTAL++))
  echo "[${TOTAL}] ${BC_FILE}" >&2

  BR_OUT="${TMP_DIR}/byterun_${TOTAL}.out"
  BR_ERR="${TMP_DIR}/byterun_${TOTAL}.err"
  LM_OUT="${TMP_DIR}/lamar_${TOTAL}.out"
  LM_ERR="${TMP_DIR}/lamar_${TOTAL}.err"

  if ! "${BYTERUN_BIN}" "${BC_FILE}" >"${BR_OUT}" 2>"${BR_ERR}"; then
    STATUS=$?
    ((FAIL++))
    ERR_MSG=$(tr -d '\r' <"${BR_ERR}" | head -c 400)
    echo "::error file=${BC_FILE}::byterun failed (exit ${STATUS}) ${ERR_MSG}" >&2
    FAILURES+=("${BC_FILE}: byterun failed")
    continue
  fi

  if ! "${LAMAR_BIN}" --print-disassemble --disassemble-only "${BC_FILE}" >"${LM_OUT}" 2>"${LM_ERR}"; then
    STATUS=$?
    ((FAIL++))
    ERR_MSG=$(tr -d '\r' <"${LM_ERR}" | head -c 400)
    echo "::error file=${BC_FILE}::lamar failed (exit ${STATUS}) ${ERR_MSG}" >&2
    FAILURES+=("${BC_FILE}: lamar failed")
    continue
  fi

  if ! diff -u "${BR_OUT}" "${LM_OUT}" >"${TMP_DIR}/diff_${TOTAL}.txt"; then
    ((FAIL++))
    echo "::error file=${BC_FILE}::Output mismatch between byterun and lamar" >&2
    cat "${TMP_DIR}/diff_${TOTAL}.txt"
    FAILURES+=("${BC_FILE}: output mismatch")
    continue
  fi

  ((PASS++))
  echo "::notice file=${BC_FILE}::PASS" >&2

done

echo "Total: ${TOTAL}, Pass: ${PASS}, Fail: ${FAIL}"

if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
  {
    echo "## Bytecode compare"
    echo "- total: ${TOTAL}"
    echo "- pass: ${PASS}"
    echo "- fail: ${FAIL}"
    if (( FAIL > 0 )); then
      echo ""
      echo "### Failures"
      for ITEM in "${FAILURES[@]}"; do
        echo "- ${ITEM}"
      done
    fi
  } >>"${GITHUB_STEP_SUMMARY}"
fi

exit $(( FAIL > 0 ? 1 : 0 ))
