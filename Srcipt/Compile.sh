#!/bin/bash

script_dir=$(cd "$(dirname "$0")" && pwd)
#echo "Script directory: $script_dir"
PROJ_PATH=$script_dir/../
#echo ${PROJ_PATH}

# 进入 CMake 目录
cd "${PROJ_PATH}/Li-Bat_MatterLight_EFR32MG24B010F1536IM40_cmake" || exit

# 定义 CMake 绝对路径
CMAKE_BIN=${HOME}/.silabs/slt/installs/conan/p/cmake1c02712053792/p/bin/cmake

# ==================== 新增：修复 build 目录问题 ====================
# 如果 build 存在但不是一个目录（可能是一个损坏的软链接或文件），强制删除它
if [ -e "build" ] && [ ! -d "build" ]; then
    echo "Warning: 'build' exists but is not a directory. Removing it..."
    rm -f build
fi
mkdir -p build
# ==================================================================

echo "Step 1: Configuring project..."
# 必须先执行配置。Simplicity SDK 的默认配置 Preset 通常叫 "project"
${CMAKE_BIN} --preset project

echo "Step 2: Building project..."
# 配置成功后，再进行编译
${CMAKE_BIN} \
    --build \
    --preset default_config \
    --parallel \
    "$(nproc)"