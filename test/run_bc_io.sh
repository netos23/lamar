#!/usr/bin/env bash
set -euo pipefail

# Run lamar for all .bc files, feed matching .input, and compare stdout with
# expected .t files (ignoring shell/prompt prefixes from the reference output).
# Environment overrides:
#   BC_DIR    - directory to scan for *.bc (default: <repo>/regression)
#   LAMAR_BIN - path to lamar executable

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BC_DIR="${BC_DIR:-${ROOT_DIR}/regression}"
LAMAR_BIN="${LAMAR_BIN:-${ROOT_DIR}/cmake-build-release/lamar}"

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

normalize_expected() {
  local src="$1" dst="$2"
  sed -E \
    -e '/^[[:space:]]*\$/{d;}' \
    -e 's/>[[:space:]]+/>/g' \
    -e 's/[[:space:]]+/ /g' \
    -e 's/^[[:space:]]+//' \
    -e 's/[[:space:]]+$//' \
    "$src" | sed '/^[[:space:]]*$/d' >"$dst"
}

normalize_actual() {
  local src="$1" dst="$2"
  sed -E \
    -e 's/>[[:space:]]+/>/g' \
    -e 's/[[:space:]]+/ /g' \
    -e 's/^[[:space:]]+//' \
    -e 's/[[:space:]]+$//' \
    "$src" | sed '/^[[:space:]]*$/d' >"$dst"
}

TOTAL=0
PASS=0
FAIL=0
FAILURES=()

echo "Running ${#BC_FILES[@]} IO checks..."

for BC_FILE in "${BC_FILES[@]}"; do
  ((TOTAL++))
  echo "[${TOTAL}] ${BC_FILE}" >&2

  DIRNAME="$(dirname "${BC_FILE}")"
  BASENAME="$(basename "${BC_FILE}" .bc)"
  INPUT_FILE="${DIRNAME}/${BASENAME}.input"
  EXPECT_FILE="${DIRNAME}/${BASENAME}.t"

  if [[ ! -f "${INPUT_FILE}" ]]; then
    ((FAIL++))
    echo "::error file=${INPUT_FILE}::input file not found" >&2
    FAILURES+=("${BC_FILE}: missing input")
    continue
  fi

  if [[ ! -f "${EXPECT_FILE}" ]]; then
    ((FAIL++))
    echo "::error file=${EXPECT_FILE}::expected .t file not found" >&2
    FAILURES+=("${BC_FILE}: missing expected")
    continue
  fi

  EXPECT_FATAL=0
  if grep -qi 'Fatal' "${EXPECT_FILE}"; then
    EXPECT_FATAL=1
  fi

  LM_RAW_OUT="${TMP_DIR}/lamar_${TOTAL}.out"
  LM_ERR="${TMP_DIR}/lamar_${TOTAL}.err"
  EXP_NORM="${TMP_DIR}/expected_${TOTAL}.txt"
  ACT_NORM="${TMP_DIR}/actual_${TOTAL}.txt"
  DIFF_FILE="${TMP_DIR}/diff_${TOTAL}.txt"

  STATUS=0
  "${LAMAR_BIN}" "${BC_FILE}" <"${INPUT_FILE}" >"${LM_RAW_OUT}" 2>"${LM_ERR}" || STATUS=$?

  if (( EXPECT_FATAL )); then
    if (( STATUS == 0 )); then
      ((FAIL++))
      echo "::error file=${BC_FILE}::Expected non-zero exit (fatal expected in ${EXPECT_FILE}). Got code ${STATUS}" >&2
      FAILURES+=("${BC_FILE}: expected fatal exit")
    else
      ((PASS++))
      echo "::notice file=${BC_FILE}::PASS (fatal expected, exit ${STATUS})" >&2
    fi
    continue
  fi

  if (( STATUS != 0 )); then
    ((FAIL++))
    ERR_MSG=$(tr -d '\r' <"${LM_ERR}" | head -c 400)
    echo "::error file=${BC_FILE}::lamar failed (exit ${STATUS}) ${ERR_MSG}" >&2
    FAILURES+=("${BC_FILE}: lamar failed")
    continue
  fi

  normalize_expected "${EXPECT_FILE}" "${EXP_NORM}"
  normalize_actual "${LM_RAW_OUT}" "${ACT_NORM}"

  if ! diff -u "${EXP_NORM}" "${ACT_NORM}" >"${DIFF_FILE}"; then
    ((FAIL++))
    echo "::error file=${BC_FILE}::Output mismatch against ${EXPECT_FILE}" >&2
    cat "${DIFF_FILE}"
    FAILURES+=("${BC_FILE}: output mismatch")
    continue
  fi

  ((PASS++))
  echo "::notice file=${BC_FILE}::PASS" >&2

done

echo "Total: ${TOTAL}, Pass: ${PASS}, Fail: ${FAIL}"

if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
  {
    echo "## Bytecode IO compare"
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
