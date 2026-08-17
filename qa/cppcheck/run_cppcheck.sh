#!/bin/bash
# cppcheck User/ — CI 无完整 Matter/SiSDK 头文件，抑制 unknownMacro 误报
# CPPCHECK_XML=1 时额外写 qa/cppcheck/report/cppcheck.xml（发版附件）
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"
report_dir="${repo_root}/qa/cppcheck/report"

cd "${repo_root}"

common_args=(
    --error-exitcode=1
    --suppress=missingIncludeSystem
    --suppress=unknownMacro
    --inline-suppr
    -I User/hal/inc
    -I User/bsp/inc
    -I User/middlewares/inc
    -I User/service/inc
    User/
)

if [ "${CPPCHECK_XML:-0}" = "1" ]; then
    mkdir -p "${report_dir}"
    cppcheck "${common_args[@]}" --xml --xml-version=2 \
        2> "${report_dir}/cppcheck.xml"
    echo "cppcheck XML: ${report_dir}/cppcheck.xml"
else
    cppcheck "${common_args[@]}"
fi

echo "cppcheck User/ OK"
