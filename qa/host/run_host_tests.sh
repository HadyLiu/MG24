#!/bin/bash
# Host 侧单元测试（无需板子 / Docker SDK）
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"
out_dir="${repo_root}/qa/host/build"
mkdir -p "${out_dir}"

cxx="${CXX:-g++}"
inc="${repo_root}/User/middlewares/inc"

echo "==== Host unit tests ===="
"${cxx}" -std=c++17 -Wall -Wextra -Werror \
    -I"${inc}" \
    -o "${out_dir}/test_light_dimming_spec" \
    "${script_dir}/test_light_dimming_spec.cpp"

"${out_dir}/test_light_dimming_spec"
echo "==== Host unit tests passed ===="
