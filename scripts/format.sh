#!/usr/bin/env bash

set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mode="${1:-format}"
formatter="${CLANG_FORMAT:-}"

if [[ -z "${formatter}" ]]; then
    for candidate in clang-format clang-format-20 clang-format-19 clang-format-18 clang-format-17; do
        if command -v "${candidate}" >/dev/null 2>&1; then
            formatter="$(command -v "${candidate}")"
            break
        fi
    done
fi

if [[ -z "${formatter}" ]] && command -v xcrun >/dev/null 2>&1; then
    formatter="$(xcrun --find clang-format 2>/dev/null || true)"
fi

if [[ -z "${formatter}" || ! -x "${formatter}" ]]; then
    echo "clang-format was not found. Install clang-format or set CLANG_FORMAT." >&2
    exit 1
fi

mapfile_command="mapfile"
if ! command -v "${mapfile_command}" >/dev/null 2>&1; then
    mapfile_command="readarray"
fi

if command -v "${mapfile_command}" >/dev/null 2>&1; then
    "${mapfile_command}" -d '' sources < <(
        find "${project_root}/src" -type f \
            \( -name '*.cpp' -o -name '*.cc' -o -name '*.cxx' -o -name '*.h' -o -name '*.hpp' \) \
            -print0
    )
else
    sources=()
    while IFS= read -r -d '' source; do
        sources+=("${source}")
    done < <(
        find "${project_root}/src" -type f \
            \( -name '*.cpp' -o -name '*.cc' -o -name '*.cxx' -o -name '*.h' -o -name '*.hpp' \) \
            -print0
    )
fi

case "${mode}" in
    format)
        "${formatter}" -i --style=file "${sources[@]}"
        ;;
    check)
        "${formatter}" --dry-run --Werror --style=file "${sources[@]}"
        ;;
    *)
        echo "Usage: $0 [format|check]" >&2
        exit 2
        ;;
esac
