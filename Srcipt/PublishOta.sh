#!/bin/bash
# 发布到宜家 HomeSmart OTA。未配置 Secrets 时跳过（不失败）。
# Secrets：IKEA_OTA_URL、IKEA_OTA_TOKEN（禁止写入仓库）
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
dist_dir="${OUT_DIR:-${repo_root}/dist}"

if [ -z "${IKEA_OTA_URL:-}" ] || [ -z "${IKEA_OTA_TOKEN:-}" ]; then
    echo "IKEA OTA skipped: set GitHub Secrets IKEA_OTA_URL and IKEA_OTA_TOKEN"
    exit 0
fi

if [ ! -d "${dist_dir}" ]; then
    echo "ERROR: dist not found: ${dist_dir}" >&2
    exit 1
fi

echo "ERROR: IKEA OTA API 协议尚未提供，拒绝用猜测的 HTTP 上传。" >&2
echo "URL is set (${IKEA_OTA_URL}) but request format is unknown." >&2
echo "Wait for C1 API docs, then fill this script." >&2
exit 1
