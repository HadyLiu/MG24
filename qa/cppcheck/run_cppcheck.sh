#!/bin/bash
# cppcheck User/ — 对齐豪庭：XML + 官方 htmlreport，目录进 code_quality_report
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"
report_dir="${repo_root}/qa/cppcheck/report"
htmlreport="${script_dir}/.tools/cppcheck-htmlreport"
xml_file="${report_dir}/cppcheck_report.xml"
text_file="${report_dir}/cppcheck.txt"
gate_rc=0

if ! command -v cppcheck >/dev/null 2>&1; then
    echo "ERROR: cppcheck not installed"
    echo "  sudo apt install cppcheck"
    exit 1
fi

cd "${repo_root}"
rm -rf "${report_dir}"
mkdir -p "${report_dir}"

chmod +x "${script_dir}/setup.sh"
"${script_dir}/setup.sh"

include_args=(
    -I User/hal/inc
    -I User/bsp/inc
    -I User/middlewares/inc
    -I User/service/inc
)

# 与豪庭 scripts/cppcheck/runner.sh 同类检查项
report_args=(
    -j"$(nproc)"
    "--enable=warning,style,performance,portability"
    --inline-suppr
    --suppress=missingIncludeSystem
    --suppress=unknownMacro
    "--suppress=*:*/test/*"
    "${include_args[@]}"
    User/
)

# 错误级门禁：不把 style 警告当成 CI 失败（豪庭 runner 也不因告警失败）
gate_args=(
    --error-exitcode=1
    --inline-suppr
    --suppress=missingIncludeSystem
    --suppress=unknownMacro
    "--suppress=*:*/test/*"
    "${include_args[@]}"
    User/
)

WriteFallbackHtml()
{
    {
        echo '<!DOCTYPE html><html lang="zh"><head><meta charset="utf-8">'
        echo '<title>cppcheck</title></head><body>'
        echo '<h1>cppcheck User/</h1><pre>'
        sed 's/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g' "${text_file}"
        echo '</pre></body></html>'
    } > "${report_dir}/index.html"
}

RunHtmlReport()
{
    if [ ! -x "${htmlreport}" ]; then
        echo "WARN: cppcheck-htmlreport missing; fallback HTML"
        WriteFallbackHtml
        return 0
    fi

    if ! python3 -c "import pygments" >/dev/null 2>&1; then
        echo "WARN: pygments missing; fallback HTML"
        WriteFallbackHtml
        return 0
    fi

    if python3 "${htmlreport}" \
        --file="${xml_file}" \
        --title=Li-Bat_MatterLight \
        --report-dir="${report_dir}" \
        --source-dir="${repo_root}"
    then
        return 0
    fi

    echo "WARN: cppcheck-htmlreport failed; fallback HTML"
    WriteFallbackHtml
}

AppendGithubSummary()
{
    if [ -z "${GITHUB_STEP_SUMMARY:-}" ]; then
        return 0
    fi
    {
        echo "## cppcheck"
        echo
        echo '```'
        cat "${text_file}"
        echo '```'
    } >> "${GITHUB_STEP_SUMMARY}"
}

set +e
cppcheck "${report_args[@]}" \
    --template='{file}:{line}: {severity}: {message}' \
    > "${text_file}" 2>&1

cppcheck "${report_args[@]}" --xml --xml-version=2 \
    > /dev/null 2> "${xml_file}"

cppcheck "${gate_args[@]}" \
    --template='{file}:{line}: {severity}: {message}' \
    > /dev/null 2>&1
gate_rc=$?
set -e

RunHtmlReport
AppendGithubSummary

echo "cppcheck HTML : ${report_dir}/index.html"
echo "cppcheck XML  : ${xml_file}"

if [ "${gate_rc}" -ne 0 ]; then
    echo "cppcheck User/ FAILED (error-level findings)"
    exit 1
fi

echo "cppcheck User/ OK"
