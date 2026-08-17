#!/bin/bash
# 准备 gcovr（豪庭 unittest/setup.sh：pip install gcovr）
set -euo pipefail

export PATH="${HOME}/.local/bin:${PATH}"

if ! command -v gcovr >/dev/null 2>&1; then
    echo "Installing gcovr"
    python3 -m pip install --user --break-system-packages gcovr
fi

if [ -n "${GITHUB_PATH:-}" ]; then
    echo "${HOME}/.local/bin" >> "${GITHUB_PATH}"
fi

if command -v gcovr >/dev/null 2>&1; then
    echo "gcovr: $(command -v gcovr)"
    exit 0
fi

if [ -n "${GITHUB_ACTIONS:-}" ]; then
    echo "ERROR: gcovr not found after setup"
    exit 1
fi

echo "WARN: gcovr not installed"
