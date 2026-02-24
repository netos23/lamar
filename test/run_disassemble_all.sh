#!/usr/bin/env bash
set -euo pipefail

# Disassemble all .bc files in the configured directories and print results to stdout.
# Environment overrides:
#   LAMAR_DISASSEMBLER_BIN - path to lamar_disassembler executable
#   BC_DIRS                - colon-separated list of directories to scan for *.bc
#                             (default: <repo>/regression:<repo>/performance)

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LAMAR_DISASSEMBLER_BIN="${LAMAR_DISASSEMBLER_BIN:-${ROOT_DIR}/cmake-build-release/lamar_disassembler}"
IFS=':' read -r -a BC_DIR_ARRAY <<<"${BC_DIRS:-${ROOT_DIR}/regression:${ROOT_DIR}/performance}"

if [[ ! -x "${LAMAR_DISASSEMBLER_BIN}" ]]; then
  echo "::error file=${LAMAR_DISASSEMBLER_BIN}::lamar_disassembler executable not found or not executable" >&2
  exit 1
fi

BC_FILES=()
for DIR in "${BC_DIR_ARRAY[@]}"; do
  if [[ ! -d "${DIR}" ]]; then
    echo "::warning file=${DIR}::.bc directory does not exist, skipping" >&2
    continue
  fi
  while IFS= read -r bc_file; do
    BC_FILES+=("${bc_file}")
  done < <(find "${DIR}" -type f -name '*.bc' | sort)
done

if [[ ${#BC_FILES[@]} -eq 0 ]]; then
  echo "::warning::No .bc files found under configured directories" >&2
  exit 0
fi

TOTAL=0
PASS=0
FAIL=0
FAILURES=()

echo "Running ${#BC_FILES[@]} disassembly checks..."

for BC_FILE in "${BC_FILES[@]}"; do
  ((++TOTAL))
  echo "[${TOTAL}] ${BC_FILE}" >&2
  echo "=== ${BC_FILE} ==="

  if ! "${LAMAR_DISASSEMBLER_BIN}" --print-disassemble "${BC_FILE}"; then
    STATUS=$?
    ((++FAIL))
    echo "::error file=${BC_FILE}::lamar_disassembler failed (exit ${STATUS})" >&2
    FAILURES+=("${BC_FILE}: lamar_disassembler failed")
    continue
  fi

  ((++PASS))
  echo "::notice file=${BC_FILE}::PASS" >&2
  echo

done

echo "Total: ${TOTAL}, Pass: ${PASS}, Fail: ${FAIL}"

if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
  {
    echo "## Disassemble all .bc"
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
