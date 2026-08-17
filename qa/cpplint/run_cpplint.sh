#!/bin/bash
# cpplint User/ — 对齐豪庭：写出 code_quality_report/cpplint/cpplint_report.xml
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"
report_dir="${repo_root}/qa/cpplint/report"
xml_file="${report_dir}/cpplint_report.xml"
lint_rc=0

cd "${repo_root}"
rm -rf "${report_dir}"
mkdir -p "${report_dir}"

chmod +x "${script_dir}/setup.sh"
"${script_dir}/setup.sh"
export PATH="${HOME}/.local/bin:${PATH}"

RunCpplint()
{
    if command -v cpplint >/dev/null 2>&1; then
        cpplint "$@"
        return $?
    fi
    python3 -m cpplint "$@"
}

mapfile -t sources < <(find User -type f \( \
    -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o \
    -name '*.h' -o -name '*.hpp' \) | sort)

if [ "${#sources[@]}" -eq 0 ]; then
    echo "ERROR: no C/C++ sources under User/"
    exit 1
fi

echo "==== cpplint User/ ===="
set +e
RunCpplint --counting=detailed "${sources[@]}" > "${xml_file}" 2>&1
lint_rc=$?
set -e

cp "${xml_file}" "${report_dir}/cpplint.txt"
cat "${xml_file}"

if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
    {
        echo "## cpplint"
        echo
        echo '```'
        tail -n 30 "${xml_file}"
        echo '```'
    } >> "${GITHUB_STEP_SUMMARY}"
fi

echo "cpplint report: ${xml_file}"

if [ "${lint_rc}" -ne 0 ]; then
    echo "cpplint User/ FAILED"
    exit 1
fi

echo "cpplint User/ OK"
