#!/bin/bash
# Host 侧单元测试：自动发现 User/**/test/*.cpp（无需板子 / Docker SDK）
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"
out_dir="${repo_root}/qa/host/build"
cxx="${CXX:-g++}"

cd "${repo_root}"
mkdir -p "${out_dir}"

mapfile -t tests < <(find User -type f -path '*/test/*.cpp' | sort)

if [ "${#tests[@]}" -eq 0 ]; then
    echo "ERROR: no host tests under User/**/test/*.cpp"
    exit 1
fi

echo "==== Host unit tests ===="
printf '  %s\n' "${tests[@]}"

inc_flags=(
    -I"${repo_root}/User/hal/inc"
    -I"${repo_root}/User/bsp/inc"
    -I"${repo_root}/User/middlewares/inc"
    -I"${repo_root}/User/service/inc"
)

for src in "${tests[@]}"; do
    name="$(basename "${src}" .cpp)"
    bin="${out_dir}/${name}"
    echo "---- ${name} ----"
    "${cxx}" -std=c++17 -Wall -Wextra -Werror \
        "${inc_flags[@]}" \
        -o "${bin}" \
        "${src}"
    "${bin}"
done

echo "==== Host unit tests passed (${#tests[@]}) ===="
