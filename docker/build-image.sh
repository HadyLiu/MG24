#!/bin/bash
# 在仓库根目录构建 Docker 镜像的便捷脚本
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
image_tag="${IMAGE_TAG:-li-bat-matterlight:sdk-2025.12.2}"
install_packages="${INSTALL_SLT_PACKAGES:-1}"

cd "${repo_root}"

echo "Build context : ${repo_root}"
echo "Dockerfile    : docker/Dockerfile"
echo "Image tag     : ${image_tag}"
echo "Install SDK   : ${install_packages}"

docker build \
    -f docker/Dockerfile \
    --build-arg "INSTALL_SLT_PACKAGES=${install_packages}" \
    -t "${image_tag}" \
    .
