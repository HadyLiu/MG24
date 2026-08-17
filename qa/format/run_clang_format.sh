#!/bin/bash
# 按仓库根 .clang-format（Google）格式化或检查 User/
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"
check_only=0

cd "${repo_root}"

if [ "${1:-}" = "--check" ] || [ "${1:-}" = "-d" ]; then
    check_only=1
fi

if ! command -v clang-format >/dev/null 2>&1; then
    echo "ERROR: clang-format not installed"
    echo "  sudo apt install clang-format"
    exit 1
fi

mapfile -t sources < <(find User -type f \( \
    -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o \
    -name '*.h' -o -name '*.hpp' \) | sort)

if [ "${#sources[@]}" -eq 0 ]; then
    echo "ERROR: no C/C++ sources under User/"
    exit 1
fi

if [ "${check_only}" -eq 1 ]; then
    echo "==== clang-format check (User/) ===="
    clang-format --dry-run --Werror "${sources[@]}"
    echo "clang-format OK"
    exit 0
fi

echo "==== clang-format -i (User/) ===="
clang-format -i --verbose "${sources[@]}"
echo "clang-format applied ${#sources[@]} files"
