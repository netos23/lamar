#!/usr/bin/env bash
set -euo pipefail

# Run lamar_analyzer for all .bc files under performance directory and print results.
# Environment overrides:
#   BC_DIR        - directory to scan for *.bc (default: <repo>/performance)
#   ANALYZER_BIN  - path to lamar_analyzer executable (default: <repo>/cmake-build-release/lamar_analyzer)

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BC_DIR="${BC_DIR:-${ROOT_DIR}/performance}"
ANALYZER_BIN="${ANALYZER_BIN:-${ROOT_DIR}/cmake-build-release/lamar_analyzer}"

if [[ ! -x "${ANALYZER_BIN}" ]]; then
  echo "::error file=${ANALYZER_BIN}::lamar_analyzer executable not found or not executable" >&2
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

TOTAL=0
FAIL=0
FAILURES=()

echo "Running lamar_analyzer for ${#BC_FILES[@]} bytecode files..." >&2

for BC_FILE in "${BC_FILES[@]}"; do
  ((++TOTAL))
  echo "[${TOTAL}] ${BC_FILE}" >&2

  if ! "${ANALYZER_BIN}" "${BC_FILE}"; then
    STATUS=$?
    ((++FAIL))
    echo "::error file=${BC_FILE}::lamar_analyzer failed (exit ${STATUS})" >&2
    FAILURES+=("${BC_FILE}: exit ${STATUS}")
  fi

done

echo "Total: ${TOTAL}, Fail: ${FAIL}" >&2

if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
  {
    echo "## Analyzer (performance)"
    echo "- total: ${TOTAL}"
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
