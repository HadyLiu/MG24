#!/bin/bash
# Tag 必须等于固件字符串版本：v + CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING
# 用法：
#   ./Srcipt/CheckReleaseTag.sh v1.1.7
#   ./Srcipt/CheckReleaseTag.sh          # 使用 GITHUB_REF_NAME
# 无 Tag（手动 workflow 且未填 tag）时跳过。
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=ReadFirmwareVersion.sh
eval "$("${script_dir}/ReadFirmwareVersion.sh")"

tag="${1:-${GITHUB_REF_NAME:-}}"

if [ -z "${tag}" ]; then
    echo "No tag given; skip version check (manual pack)."
    exit 0
fi

expected="v${FW_VERSION_STRING}"

if [ "${tag}" != "${expected}" ]; then
    echo "ERROR: tag '${tag}' != firmware '${expected}'" >&2
    echo "Update MatterLightOverThread/config/sl_matter_config.h" >&2
    echo "  CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING" >&2
    echo "  CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION" >&2
    echo "then retag. Current num=${FW_VERSION_NUM}" >&2
    exit 1
fi

echo "Tag ${tag} matches firmware ${FW_VERSION_STRING} (num=${FW_VERSION_NUM})"
