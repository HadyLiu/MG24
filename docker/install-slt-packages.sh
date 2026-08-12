#!/bin/bash
# 在镜像构建阶段由 Dockerfile 调用：经网络安装完整工具链与 SDK。
# 含重试，避免大包下载偶发 EOF。
set -euo pipefail

if [ "${INSTALL_SLT_PACKAGES:-1}" != "1" ]; then
    echo "Skip slt package install (INSTALL_SLT_PACKAGES=${INSTALL_SLT_PACKAGES})"
    exit 0
fi

CMAKE_VERSION="${CMAKE_VERSION:-3.30.2}"
NINJA_VERSION="${NINJA_VERSION:-1.12.1}"
COMMANDER_VERSION="${COMMANDER_VERSION:-1.23.1}"
GCC_ARM_VERSION="${GCC_ARM_VERSION:-12.2.rel1}"
JAVA21_VERSION="${JAVA21_VERSION:-21.0.5}"
ZAP_VERSION="${ZAP_VERSION:-2026.02.26}"
SLC_CLI_VERSION="${SLC_CLI_VERSION:-6.0.17}"
# 与当前 Silabs 在线索引可解析组合对齐（matter 2.8.1 依赖 SiSDK 2025.12.3）
SIMPLICITY_SDK_VERSION="${SIMPLICITY_SDK_VERSION:-2025.12.3}"
MATTER_EXTENSION_VERSION="${MATTER_EXTENSION_VERSION:-2.8.1}"

# Studio 导出工程硬编码的 Conan 包目录名（安装后做软链）
EXPECTED_CMAKE_HASH="${EXPECTED_CMAKE_HASH:-cmake1c02712053792}"
EXPECTED_NINJA_HASH="${EXPECTED_NINJA_HASH:-ninja1b9fed093d653}"
EXPECTED_GCC_HASH="${EXPECTED_GCC_HASH:-gcc-a442105b5c2637}"
EXPECTED_SDK_HASH="${EXPECTED_SDK_HASH:-simpl965e19baece23}"
EXPECTED_MATTER_HASH="${EXPECTED_MATTER_HASH:-mattef07831835e490}"

SltInstallRetry()
{
    local attempt=1
    local max_attempts=5
    local delay_sec=30

    while [ "${attempt}" -le "${max_attempts}" ]; do
        echo "==== slt install (attempt ${attempt}/${max_attempts}): $* ===="
        if slt --non-interactive install "$@"; then
            return 0
        fi
        echo "WARN: slt install failed (attempt ${attempt})"
        if [ "${attempt}" -eq "${max_attempts}" ]; then
            echo "ERROR: slt install failed after ${max_attempts} attempts: $*"
            return 1
        fi
        sleep "${delay_sec}"
        delay_sec=$((delay_sec + 30))
        attempt=$((attempt + 1))
    done
}

# 将 slt 实际安装目录软链到工程硬编码 hash，保证 /home/hadyliu/.silabs/... 路径可用
LinkExpectedHash()
{
    local package_id="$1"
    local expected_hash="$2"
    local installs_root="${HOME}/.silabs/slt/installs/conan/p"
    local expected_dir="${installs_root}/${expected_hash}"
    local actual_path
    local actual_hash_dir

    actual_path="$(slt where "${package_id}" 2>/dev/null || true)"
    if [ -z "${actual_path}" ] || [ ! -d "${actual_path}" ]; then
        echo "ERROR: package '${package_id}' not found after install"
        return 1
    fi

    actual_hash_dir="$(dirname "${actual_path}")"
    mkdir -p "${installs_root}"

    if [ "${actual_hash_dir}" = "${expected_dir}" ]; then
        echo "OK: ${package_id} already at ${expected_hash}"
        return 0
    fi

    echo "Link ${package_id}: ${expected_dir} -> ${actual_hash_dir}"
    rm -rf "${expected_dir}"
    ln -sfn "${actual_hash_dir}" "${expected_dir}"
}

mkdir -p "${HOME}/.silabs/slt" "${HOME}/.silabs/logs/slt"

SltInstallRetry conan

SltInstallRetry \
    "cmake/${CMAKE_VERSION}" \
    "ninja/${NINJA_VERSION}" \
    "commander/${COMMANDER_VERSION}" \
    "gcc-arm-none-eabi/${GCC_ARM_VERSION}" \
    "java21/${JAVA21_VERSION}" \
    "zap/${ZAP_VERSION}" \
    "slc-cli/${SLC_CLI_VERSION}"

# SDK + Matter 必须同一可解析组合安装，避免 Version conflict
SltInstallRetry \
    "simplicity-sdk/${SIMPLICITY_SDK_VERSION}" \
    "matter_extension/${MATTER_EXTENSION_VERSION}"

mkdir -p "${HOME}/.silabs/slt/bin"
ln -sfn "$(slt where java21)/jre/bin/java" "${HOME}/.silabs/slt/bin/java"
ln -sfn "$(slt where cmake)/bin/cmake" "${HOME}/.silabs/slt/bin/cmake"
ln -sfn "$(slt where ninja)/ninja" "${HOME}/.silabs/slt/bin/ninja"
ln -sfn "$(slt where commander)/commander" "${HOME}/.silabs/slt/bin/commander"
ln -sfn "$(slt where gcc-arm-none-eabi)/bin/arm-none-eabi-gcc" \
    "${HOME}/.silabs/slt/bin/arm-none-eabi-gcc"
ln -sfn "$(slt where zap)/zap-cli" "${HOME}/.silabs/slt/bin/zap-cli"
ln -sfn "$(slt where zap)/zap" "${HOME}/.silabs/slt/bin/zap"
printf '#!/bin/sh\nexec "%s/slc" "$@"\n' "$(slt where slc-cli)" \
    > "${HOME}/.silabs/slt/bin/slc"
chmod +x "${HOME}/.silabs/slt/bin/slc"

LinkExpectedHash "cmake" "${EXPECTED_CMAKE_HASH}"
LinkExpectedHash "ninja" "${EXPECTED_NINJA_HASH}"
LinkExpectedHash "gcc-arm-none-eabi" "${EXPECTED_GCC_HASH}"
LinkExpectedHash "simplicity-sdk" "${EXPECTED_SDK_HASH}"
LinkExpectedHash "matter_extension" "${EXPECTED_MATTER_HASH}"

echo "==== Smoke: java / slc ===="
java -version
slc --help >/dev/null
echo "slt where simplicity-sdk => $(slt where simplicity-sdk)"
echo "slt where matter_extension => $(slt where matter_extension)"
echo "expected sdk link => ${HOME}/.silabs/slt/installs/conan/p/${EXPECTED_SDK_HASH}"

# 清理下载缓存，减小镜像体积
rm -rf \
    "${HOME}/.silabs/slt/installs/archive/"*.zip \
    "${HOME}/.silabs/slt/installs/archive/"*.tar.* \
    "${HOME}/.silabs/slt/installs/conan/p/"*/d/ \
    || true

echo "==== SLC packages installed successfully ===="
