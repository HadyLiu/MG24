#!/bin/bash
# Docker 容器入口：对齐硬编码路径后执行工程编译
set -euo pipefail

PROJECT_ROOT="${PROJECT_ROOT:-/workspace}"
CMAKE_DIR="${PROJECT_ROOT}/Li-Bat_MatterLight_EFR32MG24B010F1536IM40_cmake"

# Studio 导出工程中硬编码的 Conan 包目录名（与本机一致时无需改写）
EXPECTED_CMAKE_HASH="cmake1c02712053792"
EXPECTED_NINJA_HASH="ninja1b9fed093d653"
EXPECTED_GCC_HASH="gcc-a442105b5c2637"
EXPECTED_SDK_HASH="simpl965e19baece23"
EXPECTED_MATTER_HASH="mattef07831835e490"

# 将 slt 实际安装路径软链到工程期望的 hash 目录，避免路径不一致导致编译失败
EnsureExpectedPackageLink()
{
    local package_id="$1"
    local expected_hash="$2"
    local installs_root="${HOME}/.silabs/slt/installs/conan/p"
    local expected_dir="${installs_root}/${expected_hash}"
    local actual_path=""

    if [ -d "${expected_dir}/p" ]; then
        return 0
    fi

    if ! command -v slt >/dev/null 2>&1; then
        echo "WARN: slt not found, skip link for ${package_id}"
        return 0
    fi

    actual_path="$(slt where "${package_id}" 2>/dev/null || true)"
    if [ -z "${actual_path}" ] || [ ! -d "${actual_path}" ]; then
        echo "WARN: package '${package_id}' not installed"
        return 0
    fi

    mkdir -p "${installs_root}"
    # actual_path 通常已指向 .../<hash>/p
    local actual_hash_dir
    actual_hash_dir="$(dirname "${actual_path}")"
    if [ "${actual_hash_dir}" = "${expected_dir}" ]; then
        return 0
    fi

    echo "Link ${package_id}: ${expected_dir} -> ${actual_hash_dir}"
    rm -rf "${expected_dir}"
    ln -sfn "${actual_hash_dir}" "${expected_dir}"
}

SyncHardcodedPackagePaths()
{
    if [ ! -d "${HOME}/.silabs/slt/installs" ]; then
        echo "ERROR: ${HOME}/.silabs/slt/installs not found."
        echo "Rebuild image with INSTALL_SLT_PACKAGES=1, or mount host SDK:"
        echo "  -v \"\$HOME/.silabs\":/home/hadyliu/.silabs:ro"
        exit 1
    fi

    EnsureExpectedPackageLink "cmake" "${EXPECTED_CMAKE_HASH}"
    EnsureExpectedPackageLink "ninja" "${EXPECTED_NINJA_HASH}"
    EnsureExpectedPackageLink "gcc-arm-none-eabi" "${EXPECTED_GCC_HASH}"
    EnsureExpectedPackageLink "simplicity-sdk" "${EXPECTED_SDK_HASH}"
    EnsureExpectedPackageLink "matter_extension" "${EXPECTED_MATTER_HASH}"
}

# 解析 cmake 可执行文件：优先硬编码路径，其次 slt / PATH
ResolveCmakeBin()
{
    local candidates=(
        "${HOME}/.silabs/slt/installs/conan/p/${EXPECTED_CMAKE_HASH}/p/bin/cmake"
        "${HOME}/.silabs/slt/bin/cmake"
    )
    local path=""

    for path in "${candidates[@]}"; do
        if [ -x "${path}" ]; then
            echo "${path}"
            return 0
        fi
    done

    if command -v slt >/dev/null 2>&1; then
        path="$(slt where cmake 2>/dev/null || true)/bin/cmake"
        if [ -x "${path}" ]; then
            echo "${path}"
            return 0
        fi
    fi

    if command -v cmake >/dev/null 2>&1; then
        command -v cmake
        return 0
    fi

    echo "ERROR: cmake not found" >&2
    exit 1
}

BuildProject()
{
    SyncHardcodedPackagePaths

    if [ ! -d "${CMAKE_DIR}" ]; then
        echo "ERROR: CMake project dir not found: ${CMAKE_DIR}"
        echo "Mount repository root to /workspace"
        exit 1
    fi

    local cmake_bin
    cmake_bin="$(ResolveCmakeBin)"
    echo "Using cmake: ${cmake_bin}"

    cd "${CMAKE_DIR}"

    if [ -e "build" ] && [ ! -d "build" ]; then
        echo "Warning: 'build' exists but is not a directory. Removing it..."
        rm -f build
    fi
    mkdir -p build

    echo "Step 1: Configuring project..."
    "${cmake_bin}" --preset project

    echo "Step 2: Building project..."
    "${cmake_bin}" \
        --build \
        --preset default_config \
        --parallel \
        "$(nproc)"

    echo "========================================"
    echo "   Compile success"
    echo "========================================"
    echo "Artifacts (if postbuild ran): ${PROJECT_ROOT}/artifact/"
}

# 解析 slc 可执行文件
ResolveSlcBin()
{
    local candidates=(
        "${HOME}/.silabs/slt/bin/slc"
    )
    local path=""

    for path in "${candidates[@]}"; do
        if [ -x "${path}" ]; then
            echo "${path}"
            return 0
        fi
    done

    if command -v slc >/dev/null 2>&1; then
        command -v slc
        return 0
    fi

    echo "ERROR: slc not found. Full image should install slc-cli." >&2
    exit 1
}

# 确保 java / zap / slc 软链可用（完整镜像已建好；slim 挂载宿主 SDK 时补齐）
EnsureSlcRuntime()
{
    local bin_dir="${HOME}/.silabs/slt/bin"
    mkdir -p "${bin_dir}"

    if ! command -v java >/dev/null 2>&1; then
        if command -v slt >/dev/null 2>&1; then
            local java_home=""
            java_home="$(slt where java21 2>/dev/null || true)"
            if [ -n "${java_home}" ] && [ -x "${java_home}/jre/bin/java" ]; then
                ln -sfn "${java_home}/jre/bin/java" "${bin_dir}/java"
            fi
        fi
    fi

    if ! command -v java >/dev/null 2>&1; then
        echo "ERROR: java not found. SLC CLI requires java21."
        echo "Rebuild full image, or mount host ~/.silabs that contains java21."
        exit 1
    fi

    if [ ! -x "${bin_dir}/slc" ] && command -v slt >/dev/null 2>&1; then
        local slc_home=""
        slc_home="$(slt where slc-cli 2>/dev/null || true)"
        if [ -n "${slc_home}" ] && [ -x "${slc_home}/slc" ]; then
            printf '#!/bin/sh\nexec "%s/slc" "$@"\n' "${slc_home}" > "${bin_dir}/slc"
            chmod +x "${bin_dir}/slc"
        fi
    fi

    if [ ! -x "${bin_dir}/zap-cli" ] && command -v slt >/dev/null 2>&1; then
        local zap_home=""
        zap_home="$(slt where zap 2>/dev/null || true)"
        if [ -n "${zap_home}" ] && [ -x "${zap_home}/zap-cli" ]; then
            ln -sfn "${zap_home}/zap-cli" "${bin_dir}/zap-cli"
            ln -sfn "${zap_home}/zap" "${bin_dir}/zap"
        fi
    fi

    echo "SLC runtime: java=$(command -v java)"
    java -version 2>&1 | head -1 || true
    if command -v zap-cli >/dev/null 2>&1; then
        echo "SLC runtime: zap-cli=$(command -v zap-cli)"
    else
        echo "WARN: zap-cli not in PATH (Matter generate may fail)"
    fi
}

# 对单个 .slcp 执行 SLC Generate（写回挂载的仓库目录）
GenerateOneSlcp()
{
    local project_dir="$1"
    local slcp_name="$2"
    local slc_bin="$3"
    local part="${SLC_PART:-efr32mg24b010f1536im40}"
    local timeout="${GENERATOR_TIMEOUT:-800}"
    local slcp_path="${project_dir}/${slcp_name}"

    if [ ! -f "${slcp_path}" ]; then
        echo "ERROR: slcp not found: ${slcp_path}"
        exit 1
    fi

    echo "----------------------------------------"
    echo "Generate: ${slcp_path}"
    echo "Part    : ${part}"
    echo "Timeout : ${timeout}s"
    echo "----------------------------------------"

    cd "${project_dir}"
    "${slc_bin}" generate "${slcp_name}" \
        --with "${part}" \
        --generator-timeout "${timeout}"
}

# SLC Generate：按目标刷新 Matter / Bootloader 工程文件
# 用法: generate [all|app|bootloader]
GenerateProject()
{
    local target="${1:-all}"

    SyncHardcodedPackagePaths
    EnsureSlcRuntime

    local slc_bin
    slc_bin="$(ResolveSlcBin)"
    echo "Using slc: ${slc_bin}"

    case "${target}" in
        all)
            GenerateOneSlcp \
                "${PROJECT_ROOT}/MatterLightOverThread" \
                "MatterLightOverThread.slcp" \
                "${slc_bin}"
            GenerateOneSlcp \
                "${PROJECT_ROOT}/Matter-Bootloader" \
                "Matter-Bootloader.slcp" \
                "${slc_bin}"
            ;;
        app)
            GenerateOneSlcp \
                "${PROJECT_ROOT}/MatterLightOverThread" \
                "MatterLightOverThread.slcp" \
                "${slc_bin}"
            ;;
        bootloader)
            GenerateOneSlcp \
                "${PROJECT_ROOT}/Matter-Bootloader" \
                "Matter-Bootloader.slcp" \
                "${slc_bin}"
            ;;
        *)
            echo "ERROR: unknown generate target '${target}'"
            echo "Use: generate [all|app|bootloader]"
            exit 1
            ;;
    esac

    echo "========================================"
    echo "   Generate success (${target})"
    echo "========================================"
    echo "Generated files are on the host (bind mount)."
    echo "Next: docker run ... build"
    echo "Note: keep User/ and custom ICD/OpenThread configs."
}

# 冒烟检查 SLC 工具链
CheckSlcTools()
{
    SyncHardcodedPackagePaths
    EnsureSlcRuntime
    local slc_bin
    slc_bin="$(ResolveSlcBin)"
    echo "Using slc: ${slc_bin}"
    "${slc_bin}" --help | head -20
    echo "========================================"
    echo "   SLC tools OK"
    echo "========================================"
}

ShowHelp()
{
    cat <<'EOF'
Usage:
  docker run --rm -v "$PWD":/workspace -w /workspace IMAGE [command]

Commands:
  build                 Configure + build firmware (default)
  generate [target]     SLC generate project files
                        target = all | app | bootloader  (default: all)
  check-slc             Verify java / slc / zap-cli are usable
  shell                 Interactive bash
  help                  Show this help
  <cmd...>              Run arbitrary command

Examples:
  # 0) Verify SLC toolchain
  docker run --rm -v "$PWD":/workspace -w /workspace IMAGE check-slc

  # 1) Refresh Matter + Bootloader project files (Studio Generate)
  docker run --rm -v "$PWD":/workspace -w /workspace IMAGE generate

  # 2) Only regenerate the Matter app
  docker run --rm -v "$PWD":/workspace -w /workspace IMAGE generate app

  # 3) Compile after generate / after editing User/
  docker run --rm -v "$PWD":/workspace -w /workspace IMAGE build

Env (optional):
  SLC_PART=efr32mg24b010f1536im40
  GENERATOR_TIMEOUT=800
EOF
}

main()
{
    local cmd="${1:-build}"

    case "${cmd}" in
        build)
            BuildProject
            ;;
        generate)
            shift || true
            GenerateProject "${1:-all}"
            ;;
        check-slc)
            CheckSlcTools
            ;;
        shell)
            shift || true
            exec /bin/bash "$@"
            ;;
        help|-h|--help)
            ShowHelp
            ;;
        *)
            exec "$@"
            ;;
    esac
}

main "$@"
