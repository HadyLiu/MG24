#!/bin/bash

script_dir=$(cd "$(dirname "$0")" && pwd)
#echo "Script directory: $script_dir"
PROJ_PATH=$script_dir/../
#echo ${PROJ_PATH}

cd ${PROJ_PATH}/Li-Bat_MatterLight_EFR32MG24B010F1536IM40_cmake
# 1. 执行编译
${HOME}/.silabs/slt/installs/conan/p/cmake1c02712053792/p/bin/cmake \
    --build \
    --preset default_config \
    --parallel \
    $(nproc)

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
