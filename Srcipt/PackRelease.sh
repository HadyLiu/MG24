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

FormatHex16()
{
    printf '0x%04x' "$(($1))"
}

FormatHex32()
{
    printf '0x%08x' "$1"
}

# A.B.C → 0xAABB00CC（宜家 OTA config.json，例如 1.1.5 → 0x01010005）
VersionStringToU32()
{
    local ver="$1"
    local major=""
    local minor=""
    local patch=""

    IFS='.' read -r major minor patch _ <<< "${ver}"
    if [ -z "${major}" ] || [ -z "${minor}" ]; then
        Die "bad version string: ${ver}"
    fi
    echo $(( (10#${major} << 24) | (10#${minor} << 16) | 10#${patch:-0} ))
}

# 接受 1.1.4 或 0x01010004
ParseVersionToU32()
{
    local raw="$1"

    if [[ "${raw}" =~ ^0[xX][0-9a-fA-F]+$ ]]; then
        echo $((raw))
        return 0
    fi
    VersionStringToU32 "${raw}"
}

# 读 Srcipt/OtaUpgradeRange.conf；环境变量优先
LoadOtaRangeConfRaw()
{
    local conf="${script_dir}/OtaUpgradeRange.conf"
    local key=""
    local val=""

    if [ ! -f "${conf}" ]; then
        return 0
    fi

    while IFS='=' read -r key val || [ -n "${key}" ]; do
        key="${key%%#*}"
        key="$(echo "${key}" | tr -d '[:space:]')"
        val="${val%%#*}"
        val="$(echo "${val}" | tr -d '[:space:]')"
        case "${key}" in
            OTA_MIN_VERSION)
                if [ -z "${OTA_MIN_VERSION:-}" ] && [ -n "${val}" ]; then
                    OTA_MIN_VERSION="${val}"
                fi
                ;;
            OTA_MAX_VERSION)
                if [ -z "${OTA_MAX_VERSION:-}" ] && [ -n "${val}" ]; then
                    OTA_MAX_VERSION="${val}"
                fi
                ;;
        esac
    done < "${conf}"
}

# 宜家 HomeSmart OTA 旁路元数据：productId=Matter PID
WriteOtaConfigJson()
{
    local version_u32 min_u32 max_u32
    local product_id version_hex min_hex max_hex

    LoadOtaRangeConfRaw
    version_u32="$(VersionStringToU32 "${FW_VERSION_STRING}")"
    min_u32=$((version_u32 & 0xFFFF0000))
    max_u32=$((version_u32 - 1))

    if [ -n "${OTA_MIN_VERSION:-}" ]; then
        min_u32="$(ParseVersionToU32 "${OTA_MIN_VERSION}")"
    fi
    if [ -n "${OTA_MAX_VERSION:-}" ]; then
        max_u32="$(ParseVersionToU32 "${OTA_MAX_VERSION}")"
    fi

    if [ "${min_u32}" -gt "${max_u32}" ] || [ "${max_u32}" -ge "${version_u32}" ]; then
        Die "OTA range invalid min=$(FormatHex32 "${min_u32}") max=$(FormatHex32 "${max_u32}") ver=$(FormatHex32 "${version_u32}")"
    fi

    product_id="$(FormatHex16 "${FW_PRODUCT_ID}")"
    version_hex="$(FormatHex32 "${version_u32}")"
    min_hex="$(FormatHex32 "${min_u32}")"
    max_hex="$(FormatHex32 "${max_u32}")"

    cat > "${out_dir}/config.json" <<EOF
{
    "productId": "${product_id}",
    "version": "${version_hex}",
    "minVersion": "${min_hex}",
    "maxVersion": "${max_hex}"
}
EOF
    echo "  config.json  ${product_id} ${version_hex} [${min_hex} .. ${max_hex}]"
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
WriteOtaConfigJson

(
    cd "${out_dir}"
    mapfile -t hashed < <(find . -maxdepth 1 -type f ! -name SHA256SUMS.txt | sort)
    sha256sum "${hashed[@]}" > SHA256SUMS.txt
)

echo "Packed:"
ls -lh "${out_dir}"
