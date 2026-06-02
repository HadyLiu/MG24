#!/bin/bash

script_dir=$(cd "$(dirname "$0")" && pwd)
echo "Script directory: $script_dir"
PROJ_PATH=$script_dir/../
echo ${PROJ_PATH}

rm -rf ${PROJ_PATH}Li-Bat_MatterLight_EFR32MG24B010F1536IM40_cmake/build
rm -rf ${PROJ_PATH}Matter-Bootloader/cmake_gcc/build
rm -rf ${PROJ_PATH}MatterLightOverThread/cmake_gcc/build
