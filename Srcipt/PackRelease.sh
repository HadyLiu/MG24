#!/bin/bash
# 按豪庭产物清单打包发版目录 dist/（不改 Docker 构建路径）
# 必需：BL+App full.s37、App GBL；可选：App .s37、Bootloader、Matter .ota
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
cd "${repo_root}"

# shellcheck source=ReadFirmwareVersion.sh
eval "$("${script_dir}/ReadFirmwareVersion.sh")"

out_dir="${OUT_DIR:-${repo_root}/dist}"
variant="${LI_BAT_BUILD_VARIANT:-}"
if [ -z "${variant}" ] && [ -f "${repo_root}/artifact/build_variant.txt" ]; then
    variant="$(tr -d '[:space:]' < "${repo_root}/artifact/build_variant.txt")"
fi
variant="${variant:-dev}"
prefix="${FW_PRODUCT_NAME}-v${FW_VERSION_STRING}-${variant}"

Die()
{
    echo "ERROR: $*" >&2
    exit 1
}

CopyRequired()
{
    local src="$1"
    local dest_name="$2"

    if [ ! -f "${src}" ]; then
        Die "missing required artifact: ${src}"
    fi
    cp -a "${src}" "${out_dir}/${dest_name}"
    echo "  ${dest_name}"
}

CopyOptional()
{
    local src="$1"
    local dest_name="$2"

    if [ ! -f "${src}" ]; then
        echo "WARN: skip optional ${src}"
        return 0
    fi
    cp -a "${src}" "${out_dir}/${dest_name}"
    echo "  ${dest_name}"
}

ResolveCommander()
{
    local candidates=(
        "${HOME}/.silabs/slt/bin/commander"
        "${HOME}/.silabs/slt/installs/archive/commander/commander"
    )
    local path=""

    if command -v commander >/dev/null 2>&1; then
        command -v commander
        return 0
    fi

    for path in "${candidates[@]}"; do
        if [ -x "${path}" ]; then
            echo "${path}"
            return 0
        fi
    done

    return 1
}

TryCreateMatterOta()
{
    local gbl_src="$1"
    local ota_dest="$2"
    local commander_bin=""

    if ! commander_bin="$(ResolveCommander)"; then
        echo "WARN: commander not found; skip Matter .ota (keep .gbl)"
        return 0
    fi

    echo "Create Matter OTA with ${commander_bin}"
    if "${commander_bin}" ota create --type matter \
        --input "${gbl_src}" \
        --vendorid "${FW_VENDOR_ID}" \
        --productid "${FW_PRODUCT_ID}" \
        --swstring "${FW_VERSION_STRING}" \
        --swversion "${FW_VERSION_NUM}" \
        --digest sha256 \
        -o "${ota_dest}"; then
        echo "  $(basename "${ota_dest}")"
    else
        echo "WARN: commander ota create failed; keep .gbl only"
        rm -f "${ota_dest}"
    fi
}

WriteVersionFile()
{
    local git_sha="unknown"

    if git -C "${repo_root}" rev-parse HEAD >/dev/null 2>&1; then
        git_sha="$(git -C "${repo_root}" rev-parse HEAD)"
    fi

    cat > "${out_dir}/VERSION.txt" <<EOF
product=${FW_PRODUCT_NAME}
version_string=${FW_VERSION_STRING}
version_num=${FW_VERSION_NUM}
vendor_id=${FW_VENDOR_ID}
product_id=${FW_PRODUCT_ID}
build_variant=${variant}
git_sha=${git_sha}
tag_expected=v${FW_VERSION_STRING}
EOF
}

rm -rf "${out_dir}"
mkdir -p "${out_dir}"

echo "Pack release ${prefix} -> ${out_dir}"

CopyRequired \
    "artifact/Li-Bat_MatterLight_EFR32MG24B010F1536IM40-full.s37" \
    "${prefix}-full.s37"
CopyRequired \
    "artifact/Li-Bat_MatterLight_EFR32MG24B010F1536IM40-app-only.gbl" \
    "${prefix}-app-only.gbl"

CopyOptional \
    "MatterLightOverThread/artifact/MatterLightOverThread.s37" \
    "${prefix}-app.s37"
CopyOptional \
    "Matter-Bootloader/artifact/Matter-Bootloader-crc.s37" \
    "${prefix}-bootloader.s37"

TryCreateMatterOta \
    "${out_dir}/${prefix}-app-only.gbl" \
    "${out_dir}/${prefix}-app-only.ota"

WriteVersionFile

(
    cd "${out_dir}"
    mapfile -t hashed < <(find . -maxdepth 1 -type f ! -name SHA256SUMS.txt | sort)
    sha256sum "${hashed[@]}" > SHA256SUMS.txt
)

echo "Packed:"
ls -lh "${out_dir}"
