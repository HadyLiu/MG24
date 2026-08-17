#!/bin/bash
# Host 侧单元测试：自动发现 User/**/test/*.cpp，并写出报告到 qa/host/report/
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"
out_dir="${repo_root}/qa/host/build"
report_dir="${repo_root}/qa/host/report"
log_dir="${report_dir}/logs"
cxx="${CXX:-g++}"
results_file="${report_dir}/.results"
pass_count=0
fail_count=0

cd "${repo_root}"
rm -rf "${report_dir}"
mkdir -p "${out_dir}" "${log_dir}"
: > "${results_file}"

mapfile -t tests < <(find User -type f -path '*/test/*.cpp' | sort)

if [ "${#tests[@]}" -eq 0 ]; then
    echo "ERROR: no host tests under User/**/test/*.cpp"
    exit 1
fi

inc_flags=(
    -I"${repo_root}/User/hal/inc"
    -I"${repo_root}/User/bsp/inc"
    -I"${repo_root}/User/middlewares/inc"
    -I"${repo_root}/User/service/inc"
)

RecordResult()
{
    local name="$1"
    local status="$2"
    local log_path="$3"

    echo "${name}|${status}|${log_path}" >> "${results_file}"
    if [ "${status}" = "PASS" ]; then
        pass_count=$((pass_count + 1))
    else
        fail_count=$((fail_count + 1))
    fi
}

RunOneTest()
{
    local src="$1"
    local name
    local bin
    local log_path
    local status="PASS"

    name="$(basename "${src}" .cpp)"
    bin="${out_dir}/${name}"
    log_path="${log_dir}/${name}.log"

    echo "---- ${name} ----"
    {
        echo "SOURCE ${src}"
        echo "COMPILE"
        "${cxx}" --coverage -std=c++17 -Wall -Wextra -Werror -O0 \
            "${inc_flags[@]}" \
            -o "${bin}" \
            "${src}"
        echo "RUN"
        "${bin}"
    } > "${log_path}" 2>&1 || status="FAIL"

    cat "${log_path}"
    RecordResult "${name}" "${status}" "${log_path}"
}

WriteTextReport()
{
    local line name status
    {
        echo "Host unit tests"
        echo "passed=${pass_count} failed=${fail_count}"
        echo
        while IFS='|' read -r name status _; do
            echo "${status}  ${name}"
        done < "${results_file}"
        if [ -f "${report_dir}/coverage/coverage.txt" ]; then
            echo
            echo "---- coverage ----"
            cat "${report_dir}/coverage/coverage.txt"
        fi
    } > "${report_dir}/unit_test_report.txt"
}

WriteJunitReport()
{
    local name status total
    total=$((pass_count + fail_count))
    {
        echo '<?xml version="1.0" encoding="UTF-8"?>'
        echo "<testsuites tests=\"${total}\" failures=\"${fail_count}\" name=\"host\">"
        echo "  <testsuite name=\"User.host\" tests=\"${total}\" failures=\"${fail_count}\">"
        while IFS='|' read -r name status _; do
            if [ "${status}" = "PASS" ]; then
                echo "    <testcase name=\"${name}\" classname=\"User.host\"/>"
            else
                echo "    <testcase name=\"${name}\" classname=\"User.host\">"
                echo "      <failure message=\"${name} failed\">see logs/${name}.log</failure>"
                echo "    </testcase>"
            fi
        done < "${results_file}"
        echo "  </testsuite>"
        echo "</testsuites>"
    } > "${report_dir}/junit.xml"
}

WriteHtmlReport()
{
    local name status
    {
        echo '<!DOCTYPE html>'
        echo '<html lang="zh"><head><meta charset="utf-8">'
        echo '<title>Host Unit Tests</title>'
        echo '<style>body{font-family:sans-serif;margin:24px}'
        echo 'table{border-collapse:collapse}td,th{border:1px solid #ccc;padding:6px 12px}'
        echo '.PASS{color:#0a0}.FAIL{color:#c00}</style></head><body>'
        echo "<h1>Host Unit Tests</h1>"
        echo "<p>passed=${pass_count} failed=${fail_count}</p>"
        echo '<table><tr><th>Result</th><th>Test</th><th>Log</th></tr>'
        while IFS='|' read -r name status _; do
            echo "<tr class=\"${status}\"><td>${status}</td>"
            echo "<td>${name}</td>"
            echo "<td><a href=\"logs/${name}.log\">logs/${name}.log</a></td></tr>"
        done < "${results_file}"
        echo '</table>'
        if [ -f "${report_dir}/coverage/coverage.html" ]; then
            echo '<p><a href="coverage/coverage.html">Coverage report</a></p>'
        fi
        echo '</body></html>'
    } > "${report_dir}/index.html"
}

EnsureGcovr()
{
    if command -v gcovr >/dev/null 2>&1; then
        return 0
    fi
    if [ -n "${GITHUB_ACTIONS:-}" ]; then
        echo "ERROR: gcovr required in CI (qa/host/setup.sh)"
        exit 1
    fi
    echo "WARN: gcovr not installed; skip coverage HTML"
    echo "  python3 -m pip install --user gcovr"
    return 1
}

# 对齐豪庭 codecoverage.sh -w：gcovr HTML；另写一份 txt 便于 tar 阅读
WriteCoverage()
{
    local cov_dir="${report_dir}/coverage"

    if ! EnsureGcovr; then
        return 0
    fi

    mkdir -p "${cov_dir}"
    if ! gcovr "${out_dir}" -r "${repo_root}/User" \
        -e '.*/test/.*' -e '.*/mock/.*' \
        --html-details "${cov_dir}/coverage.html"
    then
        echo "WARN: gcovr HTML failed (header-only tests may have little coverage)"
        if [ -n "${GITHUB_ACTIONS:-}" ]; then
            exit 1
        fi
        return 0
    fi
    gcovr "${out_dir}" -r "${repo_root}/User" \
        -e '.*/test/.*' -e '.*/mock/.*' \
        -o "${cov_dir}/coverage.txt"
    echo "Coverage HTML: ${cov_dir}/coverage.html"
}

AppendGithubSummary()
{
    if [ -z "${GITHUB_STEP_SUMMARY:-}" ]; then
        return 0
    fi
    {
        echo "## Host unit tests"
        echo
        echo '```'
        cat "${report_dir}/unit_test_report.txt"
        echo '```'
        echo
        echo "HTML: \`qa/host/report/index.html\`"
    } >> "${GITHUB_STEP_SUMMARY}"
}

echo "==== Host unit tests ===="
printf '  %s\n' "${tests[@]}"

for src in "${tests[@]}"; do
    RunOneTest "${src}"
done

WriteCoverage
WriteTextReport
WriteJunitReport
WriteHtmlReport
AppendGithubSummary
rm -f "${results_file}"

echo "==== Host unit tests: passed=${pass_count} failed=${fail_count} ===="
echo "Report   : ${report_dir}/index.html"
echo "Text     : ${report_dir}/unit_test_report.txt"
echo "JUnit    : ${report_dir}/junit.xml"
if [ -f "${report_dir}/coverage/coverage.html" ]; then
    echo "Coverage : ${report_dir}/coverage/coverage.html"
fi

if [ "${fail_count}" -ne 0 ]; then
    exit 1
fi
