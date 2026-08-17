#!/bin/bash
# 递增/递减固件版本（OTA 升版对）。改 sl_matter_config.h 两处宏。
# 用法：./Srcipt/UpdateFirmwareVersion.sh -i | -d
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
cfg="${repo_root}/MatterLightOverThread/config/sl_matter_config.h"
delta=0

Die()
{
    echo "ERROR: $*" >&2
    exit 1
}

usage()
{
    echo "Usage: $0 -i | -d" >&2
    exit 1
}

while getopts idh opt; do
    case "${opt}" in
        i) delta=1 ;;
        d) delta=-1 ;;
        h|*) usage ;;
    esac
done

if [ "${delta}" -eq 0 ]; then
    usage
fi

# shellcheck source=ReadFirmwareVersion.sh
eval "$("${script_dir}/ReadFirmwareVersion.sh")"

new_num=$((FW_VERSION_NUM + delta))
if [ "${new_num}" -lt 1 ]; then
    Die "software version would become ${new_num}"
fi

major="$(echo "${FW_VERSION_STRING}" | cut -d. -f1)"
minor="$(echo "${FW_VERSION_STRING}" | cut -d. -f2)"
patch="$(echo "${FW_VERSION_STRING}" | cut -d. -f3)"
new_patch=$((patch + delta))
if [ "${new_patch}" -lt 0 ]; then
    Die "patch would become ${new_patch}"
fi
new_string="${major}.${minor}.${new_patch}"

sed -i \
    "s/^#define CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION ${FW_VERSION_NUM}\$/#define CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION ${new_num}/" \
    "${cfg}"
sed -i \
    "s/^#define CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING \"${FW_VERSION_STRING}\"\$/#define CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING \"${new_string}\"/" \
    "${cfg}"

echo "Firmware version ${FW_VERSION_NUM}/${FW_VERSION_STRING} -> ${new_num}/${new_string}"
