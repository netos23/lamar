#!/usr/bin/env bash
set -euo pipefail

LAMAC_BIN="${LAMAC_BIN:-lamac}"
while getopts "d" opt; do
  case "$opt" in
    d) LAMAC_BIN="/home/user/.opam/4.14.2/bin/lamac" ;;
    *) echo "Usage: $0 [-d]" >&2; exit 1 ;;
  esac
done
shift $((OPTIND - 1))

for file in ./*.lama; do
  [ -e "$file" ] || continue
  if ! "$LAMAC_BIN" -b "$file"; then
    echo "warning: Can't produce bytecode for $file" >&2
    continue
  fi
done
