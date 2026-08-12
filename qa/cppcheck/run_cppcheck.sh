#!/bin/bash
# cppcheck User/ — CI 无完整 Matter/SiSDK 头文件，抑制 unknownMacro 误报
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"

cd "${repo_root}"

cppcheck \
    --error-exitcode=1 \
    --suppress=missingIncludeSystem \
    --suppress=unknownMacro \
    --inline-suppr \
    -I User/hal/inc \
    -I User/bsp/inc \
    -I User/middlewares/inc \
    -I User/service/inc \
    User/

echo "cppcheck User/ OK"
