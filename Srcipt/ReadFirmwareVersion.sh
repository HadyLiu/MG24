#!/bin/bash
# 从固件头文件读出版本 / VID / PID（发版与打包的单一来源）
# 用法：eval "$(./Srcipt/ReadFirmwareVersion.sh)"
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

matter_cfg="${repo_root}/MatterLightOverThread/config/sl_matter_config.h"
chip_cfg="${repo_root}/MatterLightOverThread/include/CHIPProjectConfig.h"

Die()
{
    echo "ERROR: $*" >&2
    exit 1
}

ReadDefineValue()
{
    local file="$1"
    local name="$2"
    local line=""

    if [ ! -f "${file}" ]; then
        Die "missing ${file}"
    fi

    line="$(grep -E "^#define ${name}[[:space:]]" "${file}" | head -n 1 || true)"
    if [ -z "${line}" ]; then
        Die "macro ${name} not found in ${file}"
    fi

    echo "${line}" | awk '{print $3}' | tr -d '"'
}

fw_version_num="$(ReadDefineValue "${matter_cfg}" \
    "CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION")"
fw_version_string="$(ReadDefineValue "${matter_cfg}" \
    "CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING")"
fw_vendor_id="$(ReadDefineValue "${chip_cfg}" \
    "CHIP_DEVICE_CONFIG_DEVICE_VENDOR_ID")"
fw_product_id="$(ReadDefineValue "${chip_cfg}" \
    "CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID")"

if [ -z "${fw_version_num}" ] || [ -z "${fw_version_string}" ]; then
    Die "empty firmware version in ${matter_cfg}"
fi

cat <<EOF
FW_VERSION_NUM=${fw_version_num}
FW_VERSION_STRING=${fw_version_string}
FW_VENDOR_ID=${fw_vendor_id}
FW_PRODUCT_ID=${fw_product_id}
FW_PRODUCT_NAME=Li-Bat_MatterLight
EOF
