# =============================================================================
# Li-Bat MatterLight (EFR32MG24) — 完整固件编译 + SLC CLI 镜像
#
# 特点：
#   - 仅通过网络安装 Silabs 工具链与 SDK（不依赖宿主机 ~/.silabs）
#   - 镜像内用户固定为 hadyliu，匹配 Studio 导出工程硬编码路径
#   - 内置 java21 / zap / slc-cli，支持 generate + build
#
# ---------------------------------------------------------------------------
# 他人 / CI 构建完整镜像（仓库根目录）:
#   docker build -f docker/Dockerfile -t li-bat-matterlight:sdk-2025.12.2 .
#
# 或:
#   ./docker/build-image.sh
#
# 编译固件（无需挂载宿主 SDK）:
#   docker run --rm -v "$PWD":/workspace -w /workspace \
#     li-bat-matterlight:sdk-2025.12.2 build
#
# SLC 自检:
#   docker run --rm -v "$PWD":/workspace -w /workspace \
#     li-bat-matterlight:sdk-2025.12.2 check-slc
#
# 精简镜像（跳过 SDK，需自行挂载宿主 ~/.silabs）:
#   docker build -f docker/Dockerfile --build-arg INSTALL_SLT_PACKAGES=0 \
#     -t li-bat-matterlight:slim .
# =============================================================================

FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
ARG BUILD_USER=hadyliu
ARG BUILD_UID=1000
ARG BUILD_GID=1000

ARG SLT_CLI_URL=https://www.silabs.com/documents/public/software/slt-cli-1.1.0-linux-x64.zip
ARG INSTALL_SLT_PACKAGES=1

# 工具链版本（可 --build-arg 覆盖）
ARG CMAKE_VERSION=3.30.2
ARG NINJA_VERSION=1.12.1
ARG COMMANDER_VERSION=1.23.1
ARG GCC_ARM_VERSION=12.2.rel1
ARG JAVA21_VERSION=21.0.5
ARG ZAP_VERSION=2026.02.26
ARG SLC_CLI_VERSION=6.0.17
# matter_extension/2.8.1 与在线索引解析时需 SiSDK 2025.12.3（同装避免 Version conflict）
ARG SIMPLICITY_SDK_VERSION=2025.12.3
ARG MATTER_EXTENSION_VERSION=2.8.1

# Studio 工程硬编码 Conan 目录名（install 脚本会软链到实际安装路径）
ARG EXPECTED_CMAKE_HASH=cmake1c02712053792
ARG EXPECTED_NINJA_HASH=ninja1b9fed093d653
ARG EXPECTED_GCC_HASH=gcc-a442105b5c2637
ARG EXPECTED_SDK_HASH=simpl965e19baece23
ARG EXPECTED_MATTER_HASH=mattef07831835e490

ENV LANG=C.UTF-8 \
    LC_ALL=C.UTF-8 \
    PATH="/home/${BUILD_USER}/.silabs/slt/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"

RUN set -eux \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        aria2 \
        bzip2 \
        ca-certificates \
        curl \
        git \
        jq \
        libarchive-tools \
        libasound2t64 \
        libatk-bridge2.0-0t64 \
        libatk1.0-0t64 \
        libdrm2 \
        libgbm1 \
        libgl1 \
        libglib2.0-0t64 \
        libgtk-3-0t64 \
        libnss3 \
        libpcre2-16-0 \
        libpng16-16t64 \
        libstdc++6 \
        libx11-6 \
        libxkbcommon0 \
        libxcomposite1 \
        libxdamage1 \
        libxfixes3 \
        libxrandr2 \
        python3 \
        python3-pip \
        unzip \
        xz-utils \
    && rm -rf /var/lib/apt/lists/*

# 创建构建用户，并预先准备可写的 ~/.silabs（勿在 root 阶段用该 HOME 跑 slt）
RUN set -eux \
    && if id -u ubuntu >/dev/null 2>&1; then userdel -r ubuntu || true; fi \
    && groupadd --gid "${BUILD_GID}" "${BUILD_USER}" \
    && useradd --uid "${BUILD_UID}" --gid "${BUILD_GID}" \
        --create-home --home-dir "/home/${BUILD_USER}" \
        --shell /bin/bash "${BUILD_USER}" \
    && mkdir -p \
         "/home/${BUILD_USER}/.silabs/slt" \
         "/home/${BUILD_USER}/.silabs/logs/slt" \
    && chown -R "${BUILD_USER}:${BUILD_USER}" "/home/${BUILD_USER}"

# 安装 slt CLI（强制 HOME=/root，避免污染构建用户家目录）
RUN set -eux \
    && aria2c --allow-overwrite=true -o /tmp/slt.zip "${SLT_CLI_URL}" \
    && bsdtar -xf /tmp/slt.zip -C /usr/local/bin \
    && rm -f /tmp/slt.zip \
    && chmod +x /usr/local/bin/slt \
    && HOME=/root slt --version

COPY --chown=${BUILD_USER}:${BUILD_USER} docker/install-slt-packages.sh \
    /usr/local/bin/install-slt-packages.sh
# 去掉可能的 CRLF，避免 shebang 变成 /bin/bash\r 导致 not found
RUN sed -i 's/\r$//' /usr/local/bin/install-slt-packages.sh \
    && chmod +x /usr/local/bin/install-slt-packages.sh

USER ${BUILD_USER}
ENV HOME=/home/${BUILD_USER}
WORKDIR /home/${BUILD_USER}

# 经网络安装完整工具链 / SDK / Matter / SLC（可 INSTALL_SLT_PACKAGES=0 跳过）
ARG INSTALL_SLT_PACKAGES
ARG CMAKE_VERSION
ARG NINJA_VERSION
ARG COMMANDER_VERSION
ARG GCC_ARM_VERSION
ARG JAVA21_VERSION
ARG ZAP_VERSION
ARG SLC_CLI_VERSION
ARG SIMPLICITY_SDK_VERSION
ARG MATTER_EXTENSION_VERSION
ARG EXPECTED_CMAKE_HASH
ARG EXPECTED_NINJA_HASH
ARG EXPECTED_GCC_HASH
ARG EXPECTED_SDK_HASH
ARG EXPECTED_MATTER_HASH

RUN INSTALL_SLT_PACKAGES="${INSTALL_SLT_PACKAGES}" \
    CMAKE_VERSION="${CMAKE_VERSION}" \
    NINJA_VERSION="${NINJA_VERSION}" \
    COMMANDER_VERSION="${COMMANDER_VERSION}" \
    GCC_ARM_VERSION="${GCC_ARM_VERSION}" \
    JAVA21_VERSION="${JAVA21_VERSION}" \
    ZAP_VERSION="${ZAP_VERSION}" \
    SLC_CLI_VERSION="${SLC_CLI_VERSION}" \
    SIMPLICITY_SDK_VERSION="${SIMPLICITY_SDK_VERSION}" \
    MATTER_EXTENSION_VERSION="${MATTER_EXTENSION_VERSION}" \
    EXPECTED_CMAKE_HASH="${EXPECTED_CMAKE_HASH}" \
    EXPECTED_NINJA_HASH="${EXPECTED_NINJA_HASH}" \
    EXPECTED_GCC_HASH="${EXPECTED_GCC_HASH}" \
    EXPECTED_SDK_HASH="${EXPECTED_SDK_HASH}" \
    EXPECTED_MATTER_HASH="${EXPECTED_MATTER_HASH}" \
    /usr/local/bin/install-slt-packages.sh

USER root
COPY --chown=${BUILD_USER}:${BUILD_USER} docker/entrypoint.sh /usr/local/bin/entrypoint.sh
RUN sed -i 's/\r$//' /usr/local/bin/entrypoint.sh \
    && chmod +x /usr/local/bin/entrypoint.sh

USER ${BUILD_USER}
WORKDIR /workspace

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
CMD ["build"]
