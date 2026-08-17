#!/bin/bash
# 本地一键复现 CI：lint + QA + Docker 固件编译
# 用法（仓库根目录）:
#   ./Srcipt/CiLocal.sh
#   ./Srcipt/CiLocal.sh --lint-only
#   ./Srcipt/CiLocal.sh --qa-only
#   ./Srcipt/CiLocal.sh --build-only
#   IMAGE=li-bat-matterlight:slim ./Srcipt/CiLocal.sh --build-only
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
cd "${repo_root}"

IMAGE="${IMAGE:-li-bat-matterlight:sdk-2025.12.2}"
DO_LINT=1
DO_QA=1
DO_BUILD=1

for arg in "$@"; do
    case "${arg}" in
        --lint-only)
            DO_QA=0
            DO_BUILD=0
            ;;
        --qa-only)
            DO_LINT=0
            DO_BUILD=0
            ;;
        --build-only)
            DO_LINT=0
            DO_QA=0
            ;;
        -h|--help)
            cat <<'EOF'
Usage: ./Srcipt/CiLocal.sh [--lint-only|--qa-only|--build-only]

Env:
  IMAGE   Docker image (default: li-bat-matterlight:sdk-2025.12.2)
          slim: IMAGE=li-bat-matterlight:slim ./Srcipt/CiLocal.sh --build-only
  LI_BAT_BUILD_VARIANT  dev (default) | release
EOF
            exit 0
            ;;
        *)
            echo "ERROR: unknown arg: ${arg}"
            exit 1
            ;;
    esac
done

RunLint()
{
    echo "==== Lint (local) ===="
    local scripts=(
        docker/entrypoint.sh
        docker/build-image.sh
        docker/install-slt-packages.sh
        Srcipt/CiLocal.sh
        Srcipt/ReadFirmwareVersion.sh
        Srcipt/CheckReleaseTag.sh
        Srcipt/PackRelease.sh
        Srcipt/UpdateFirmwareVersion.sh
        Srcipt/PublishOta.sh
        qa/host/run_host_tests.sh
        qa/host/setup.sh
        qa/cppcheck/run_cppcheck.sh
        qa/cppcheck/setup.sh
        qa/cpplint/run_cpplint.sh
        qa/cpplint/setup.sh
        qa/format/run_clang_format.sh
        qa/PackQualityReports.sh
    )

    if command -v shellcheck >/dev/null 2>&1; then
        shellcheck --severity=warning "${scripts[@]}"
        echo "shellcheck OK"
    else
        echo "WARN: shellcheck not installed (skip)."
    fi

    if command -v yamllint >/dev/null 2>&1; then
        yamllint -c .github/yamllint.yml .github/workflows/
        echo "yamllint OK"
    else
        echo "WARN: yamllint not installed (skip)."
    fi

    if command -v actionlint >/dev/null 2>&1; then
        actionlint -color
        echo "actionlint OK"
    else
        echo "WARN: actionlint not installed (skip)."
    fi
}

RunQa()
{
    echo "==== QA (host tests / Google style / cppcheck) ===="
    chmod +x qa/host/run_host_tests.sh qa/host/setup.sh
    chmod +x qa/cppcheck/run_cppcheck.sh qa/cppcheck/setup.sh
    chmod +x qa/cpplint/run_cpplint.sh qa/cpplint/setup.sh
    chmod +x qa/format/run_clang_format.sh qa/PackQualityReports.sh

    if command -v clang-format >/dev/null 2>&1; then
        ./qa/format/run_clang_format.sh --check
    else
        echo "WARN: clang-format not installed (skip). Install: sudo apt install clang-format"
    fi

    ./qa/host/setup.sh || echo "WARN: qa/host/setup.sh failed (gcovr optional locally)"
    export PATH="${HOME}/.local/bin:${PATH}"
    ./qa/host/run_host_tests.sh

    ./qa/cpplint/run_cpplint.sh

    if command -v cppcheck >/dev/null 2>&1; then
        ./qa/cppcheck/run_cppcheck.sh
        ./qa/PackQualityReports.sh qa
    else
        echo "WARN: cppcheck not installed (skip). Install: sudo apt install cppcheck"
        ./qa/PackQualityReports.sh --unit-only qa
    fi

    echo "QA reports:"
    echo "  qa/host/report/index.html"
    echo "  qa/host/report/coverage/coverage.html"
    echo "  qa/cppcheck/report/index.html"
    echo "  qa/cpplint/report/cpplint_report.xml"
    echo "  qa/unit_test_report.tar.gz"
    echo "  qa/code_quality_report.tar.gz"
}

RunBuild()
{
    echo "==== Firmware build (Docker) ===="
    if ! command -v docker >/dev/null 2>&1; then
        echo "ERROR: docker not found"
        exit 1
    fi

    if ! docker image inspect "${IMAGE}" >/dev/null 2>&1; then
        echo "ERROR: image not found locally: ${IMAGE}"
        echo "Build: ./docker/build-image.sh"
        exit 1
    fi

    extra_mounts=()
    if [[ "${IMAGE}" == *":slim"* ]]; then
        if [ ! -d "${HOME}/.silabs" ]; then
            echo "ERROR: slim image requires ${HOME}/.silabs"
            exit 1
        fi
        extra_mounts+=(-v "${HOME}/.silabs:/home/hadyliu/.silabs:ro")
    fi

    docker run --rm \
        -e LI_BAT_BUILD_VARIANT="${LI_BAT_BUILD_VARIANT:-dev}" \
        -v "${repo_root}:/workspace" \
        "${extra_mounts[@]}" \
        -w /workspace \
        "${IMAGE}" \
        build

    docker run --rm \
        -e LI_BAT_BUILD_VARIANT="${LI_BAT_BUILD_VARIANT:-dev}" \
        -v "${repo_root}:/workspace" \
        "${extra_mounts[@]}" \
        -w /workspace \
        "${IMAGE}" \
        pack

    echo "Artifacts:"
    ls -lh artifact/*.{s37,gbl} 2>/dev/null || true
    echo "Release dist:"
    ls -lh dist/ 2>/dev/null || true
}

if [ "${DO_LINT}" -eq 1 ]; then
    RunLint
fi

if [ "${DO_QA}" -eq 1 ]; then
    RunQa
fi

if [ "${DO_BUILD}" -eq 1 ]; then
    RunBuild
fi

echo "==== CiLocal done ===="
