#!/bin/bash
# 准备 cpplint（豪庭 clone 仓库；本仓 pip 安装）
set -euo pipefail

export PATH="${HOME}/.local/bin:${PATH}"

if python3 -c "import cpplint" >/dev/null 2>&1 || command -v cpplint >/dev/null 2>&1; then
    echo "cpplint already available"
else
    echo "Installing cpplint"
    python3 -m pip install --user --break-system-packages cpplint
fi

if [ -n "${GITHUB_PATH:-}" ]; then
    echo "${HOME}/.local/bin" >> "${GITHUB_PATH}"
fi

if command -v cpplint >/dev/null 2>&1; then
    echo "cpplint: $(command -v cpplint)"
    exit 0
fi

if python3 -c "import cpplint" >/dev/null 2>&1; then
    echo "cpplint: python3 -m cpplint"
    exit 0
fi

if [ -n "${GITHUB_ACTIONS:-}" ]; then
    echo "ERROR: cpplint not found after setup"
    exit 1
fi

echo "WARN: cpplint not installed"
exit 1
