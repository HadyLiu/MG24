#!/bin/bash

script_dir=$(cd "$(dirname "$0")" && pwd)
#echo "Script directory: $script_dir"
PROJ_PATH=$script_dir/../
#echo ${PROJ_PATH}

${HOME}/.silabs/slt/installs/archive/commander/commander \
    flash \
    "${PROJ_PATH}/artifact/Li-Bat_MatterLight_EFR32MG24B010F1536IM40-app-only.gbl"\
    --serialno 440350761 \
    --device EFR32MG24B010F1536IM40 \
    --speed 10000
