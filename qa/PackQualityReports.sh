#!/bin/bash
# 按豪庭约定打包质量报告 tar.gz
#   code_quality_report.tar.gz  → code_quality_report/cppcheck/
#   unit_test_report.tar.gz     → unit_test_report/
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
pack_unit=1
pack_quality=1
out_dir="${repo_root}/dist"
staging="${repo_root}/qa/.pack_staging"

PrintUsage()
{
    cat <<'EOF'
Usage: ./qa/PackQualityReports.sh [--unit-only|--quality-only] [OUT_DIR]

Default OUT_DIR: dist/
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --unit-only)
            pack_quality=0
            shift
            ;;
        --quality-only)
            pack_unit=0
            shift
            ;;
        -h|--help)
            PrintUsage
            exit 0
            ;;
        *)
            out_dir="$1"
            shift
            ;;
    esac
done

if [[ "${out_dir}" != /* ]]; then
    out_dir="${repo_root}/${out_dir}"
fi

mkdir -p "${out_dir}"
rm -rf "${staging}"
mkdir -p "${staging}"

PackUnitReport()
{
    local src="${repo_root}/qa/host/report"

    if [ ! -f "${src}/unit_test_report.txt" ]; then
        echo "ERROR: missing ${src}/unit_test_report.txt"
        exit 1
    fi

    mkdir -p "${staging}/unit_test_report"
    cp -a "${src}/." "${staging}/unit_test_report/"
    tar -C "${staging}" -czf "${out_dir}/unit_test_report.tar.gz" unit_test_report
    echo "Wrote ${out_dir}/unit_test_report.tar.gz"
}

PackQualityReport()
{
    local cppcheck_src="${repo_root}/qa/cppcheck/report"
    local cpplint_src="${repo_root}/qa/cpplint/report"

    if [ ! -f "${cppcheck_src}/cppcheck_report.xml" ]; then
        echo "ERROR: missing ${cppcheck_src}/cppcheck_report.xml"
        exit 1
    fi

    mkdir -p "${staging}/code_quality_report/cppcheck"
    cp -a "${cppcheck_src}/." "${staging}/code_quality_report/cppcheck/"

    if [ -f "${cpplint_src}/cpplint_report.xml" ]; then
        mkdir -p "${staging}/code_quality_report/cpplint"
        cp -a "${cpplint_src}/." "${staging}/code_quality_report/cpplint/"
    elif [ -n "${GITHUB_ACTIONS:-}" ]; then
        echo "ERROR: missing ${cpplint_src}/cpplint_report.xml"
        exit 1
    else
        echo "WARN: no cpplint report (skip packing cpplint/)"
    fi

    tar -C "${staging}" -czf "${out_dir}/code_quality_report.tar.gz" \
        code_quality_report
    echo "Wrote ${out_dir}/code_quality_report.tar.gz"
}

if [ "${pack_unit}" -eq 1 ]; then
    PackUnitReport
fi

if [ "${pack_quality}" -eq 1 ]; then
    PackQualityReport
fi
