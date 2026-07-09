#!/bin/bash

script_dir=$(cd "$(dirname "$0")" && pwd)
#echo "Script directory: $script_dir"
PROJ_PATH=$script_dir/../
#echo ${PROJ_PATH}

#################################################
### 编译文件
#################################################
# 进入 CMake 目录
cd ${PROJ_PATH}/Li-Bat_MatterLight_EFR32MG24B010F1536IM40_cmake

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
# 1. 执行编译
${CMAKE_BIN} \
    --build \
    --preset default_config \
    --parallel \
    $(nproc)

#################################################
### 判断编译是否成功 若成功则执行下载
#################################################

# 2. 判断编译是否成功 ($? 为 0 表示成功)
if [ $? -eq 0 ]; then
    echo "========================================"
    echo "   Compile success! Starting flash...   "
    echo "========================================"
    
    # 3. 编译成功，执行下载
    ${HOME}/.silabs/slt/installs/archive/commander/commander \
        flash \
        "${PROJ_PATH}/artifact/Li-Bat_MatterLight_EFR32MG24B010F1536IM40-app-only.gbl" \
        --serialno 440350761 \
        --device EFR32MG24B010F1536IM40 \
        --speed 10000
else
    echo "========================================"
    echo "   Compile FAILED. Flash aborted!       "
    echo "========================================"
    exit 1
fi
