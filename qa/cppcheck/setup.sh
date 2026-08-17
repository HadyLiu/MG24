#!/bin/bash
# 准备官方 cppcheck-htmlreport（豪庭从源码自带；本仓不编译 cppcheck）
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
tools_dir="${script_dir}/.tools"
htmlreport="${tools_dir}/cppcheck-htmlreport"
htmlreport_url="https://raw.githubusercontent.com/danmar/cppcheck/2.17.1/htmlreport/cppcheck-htmlreport"

mkdir -p "${tools_dir}"

EnsurePygments()
{
    if python3 -c "import pygments" >/dev/null 2>&1; then
        return 0
    fi

    echo "Installing pygments for cppcheck-htmlreport"
    if python3 -m pip install --user --break-system-packages pygments; then
        return 0
    fi
    if [ -n "${GITHUB_ACTIONS:-}" ]; then
        echo "ERROR: pygments install failed"
        exit 1
    fi
    echo "WARN: pygments not available; htmlreport may fall back"
}

DownloadHtmlReport()
{
    local tmp_file

    if [ -x "${htmlreport}" ]; then
        return 0
    fi

    echo "Downloading cppcheck-htmlreport"
    tmp_file="${htmlreport}.tmp"
    if curl -fsSL --retry 5 --retry-delay 2 \
        -o "${tmp_file}" "${htmlreport_url}"
    then
        chmod +x "${tmp_file}"
        mv "${tmp_file}" "${htmlreport}"
        return 0
    fi

    rm -f "${tmp_file}"
    if [ -n "${GITHUB_ACTIONS:-}" ]; then
        echo "ERROR: failed to download cppcheck-htmlreport"
        exit 1
    fi
    echo "WARN: cppcheck-htmlreport download failed; will use fallback HTML"
}

EnsurePygments
DownloadHtmlReport

if [ -n "${GITHUB_PATH:-}" ]; then
    echo "${tools_dir}" >> "${GITHUB_PATH}"
    echo "${HOME}/.local/bin" >> "${GITHUB_PATH}"
fi

echo "cppcheck-htmlreport: ${htmlreport}"
