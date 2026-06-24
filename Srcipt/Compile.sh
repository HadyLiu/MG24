#!/bin/bash

script_dir=$(cd "$(dirname "$0")" && pwd)
#echo "Script directory: $script_dir"
PROJ_PATH=$script_dir/../
#echo ${PROJ_PATH}

cd ${PROJ_PATH}/Li-Bat_MatterLight_EFR32MG24B010F1536IM40_cmake

${HOME}/.silabs/slt/installs/conan/p/cmake1c02712053792/p/bin/cmake \
    --build \
    --preset default_config \
    --parallel \
    $(nproc)
