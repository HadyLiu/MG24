####################################################################
# Automatically-generated file. Do not edit!                       #
####################################################################

set(SDK_PATH "/home/hadyliu/.silabs/slt/installs/conan/p/simpl965e19baece23/p")
set(COPIED_SDK_PATH "simplicity_sdk_2025.12.2")
set(PKG_PATH "/home/hadyliu/.silabs/slt/installs")

add_library(slc OBJECT
    "${SDK_PATH}/../../mattef07831835e490/p/third_party/matter_sdk/examples/platform/silabs/syscalls_stubs.cpp"
    "${SDK_PATH}/bootloader/platform/bootloader/core/btl_bootload.c"
    "${SDK_PATH}/bootloader/platform/bootloader/core/btl_core.c"
    "${SDK_PATH}/bootloader/platform/bootloader/core/btl_main.c"
    "${SDK_PATH}/bootloader/platform/bootloader/core/btl_parse.c"
    "${SDK_PATH}/bootloader/platform/bootloader/core/btl_reset.c"
    "${SDK_PATH}/bootloader/platform/bootloader/core/flash/btl_internal_flash.c"
    "${SDK_PATH}/bootloader/platform/bootloader/debug/btl_debug.c"
    "${SDK_PATH}/bootloader/platform/bootloader/debug/btl_debug_swo.c"
    "${SDK_PATH}/bootloader/platform/bootloader/driver/btl_driver_util.c"
    "${SDK_PATH}/bootloader/platform/bootloader/parser/compression/btl_decompress_lz4.c"
    "${SDK_PATH}/bootloader/platform/bootloader/parser/compression/btl_decompress_lzma.c"
    "${SDK_PATH}/bootloader/platform/bootloader/parser/compression/lzma/LzmaDec.c"
    "${SDK_PATH}/bootloader/platform/bootloader/parser/gbl/btl_gbl_custom_tags.c"
    "${SDK_PATH}/bootloader/platform/bootloader/parser/gbl/btl_gbl_format.c"
    "${SDK_PATH}/bootloader/platform/bootloader/parser/gbl/btl_gbl_parser.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/btl_crc16.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/btl_crc32.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/btl_security_aes.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/btl_security_ecdsa.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/btl_security_sha256.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/btl_security_tokens.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/ecc/ecc.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/sha/btl_sha256.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/sha/crypto_sha.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/sha/cryptoacc_sha.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/sha/se_sha.c"
    "${SDK_PATH}/bootloader/platform/bootloader/storage/bootloadinfo/btl_storage_bootloadinfo.c"
    "${SDK_PATH}/bootloader/platform/bootloader/storage/btl_storage.c"
    "${SDK_PATH}/bootloader/platform/bootloader/storage/btl_storage_library.c"
    "${SDK_PATH}/bootloader/platform/bootloader/storage/internal_flash/btl_storage_internal_flash.c"
    "${SDK_PATH}/bootloader/platform/bootloader/storage/internal_flash/btl_storage_internal_flash_raw.c"
    "${SDK_PATH}/devices/platform/Device/SiliconLabs/EFR32MG24/Source/startup_efr32mg24.c"
    "${SDK_PATH}/devices/platform/Device/SiliconLabs/EFR32MG24/Source/system_efr32mg24.c"
    "${SDK_PATH}/platform_common/platform/common/src/sl_assert.c"
    "${SDK_PATH}/platform_common/platform/common/src/sl_syscalls.c"
    "${SDK_PATH}/platform_core/platform/common/src/sl_core_cortexm.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_acmp.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_burtc.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_cmu.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_dbg.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_emu.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_eusart.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_gpcrc.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_gpio.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_i2c.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_iadc.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_ldma.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_letimer.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_msc.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_opamp.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_pcnt.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_prs.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_rmu.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_system.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_timer.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_usart.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_vdac.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_wdog.c"
    "${SDK_PATH}/platform_core/platform/service/device_init/src/sl_device_init_lfrco.c"
    "${SDK_PATH}/platform_core/platform/service/memory_manager/src/sl_memory_manager.c"
    "${SDK_PATH}/platform_core/platform/service/memory_manager/src/sl_memory_manager_dynamic_reservation.c"
    "${SDK_PATH}/platform_core/platform/service/memory_manager/src/sl_memory_manager_pool.c"
    "${SDK_PATH}/platform_core/platform/service/memory_manager/src/sl_memory_manager_pool_common.c"
    "${SDK_PATH}/platform_core/platform/service/memory_manager/src/sl_memory_manager_region.c"
    "${SDK_PATH}/platform_core/platform/service/memory_manager/src/sl_memory_manager_retarget.c"
    "${SDK_PATH}/platform_core/platform/service/memory_manager/src/sli_memory_manager_common.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_mbedtls_support/src/se_aes.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_mbedtls_support/src/sl_mbedtls.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_psa_driver_common.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_psa_driver_init.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_driver_aead.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_driver_builtin_keys.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_driver_cipher.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_driver_key_derivation.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_driver_key_management.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_driver_mac.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_driver_signature.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_opaque_driver_aead.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_opaque_driver_cipher.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_opaque_driver_mac.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_opaque_key_derivation.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_transparent_driver_aead.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_transparent_driver_cipher.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_transparent_driver_hash.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_transparent_driver_mac.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_transparent_key_derivation.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_version_dependencies.c"
    "${SDK_PATH}/security_mbedtls_source/library/aes.c"
    "${SDK_PATH}/security_mbedtls_source/library/constant_time.c"
    "${SDK_PATH}/security_mbedtls_source/library/platform.c"
    "${SDK_PATH}/security_mbedtls_source/library/platform_util.c"
    "${SDK_PATH}/security_mbedtls_source/library/psa_crypto_client.c"
    "${SDK_PATH}/security_mbedtls_source/library/psa_util.c"
    "${SDK_PATH}/security_mbedtls_source/library/threading.c"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/src/sl_se_manager.c"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/src/sl_se_manager_attestation.c"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/src/sl_se_manager_cipher.c"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/src/sl_se_manager_entropy.c"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/src/sl_se_manager_hash.c"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/src/sl_se_manager_key_derivation.c"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/src/sl_se_manager_key_handling.c"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/src/sl_se_manager_signature.c"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/src/sl_se_manager_util.c"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/src/sli_se_manager_mailbox.c"
    "${SDK_PATH}/security_tfm/lib/fih/src/fih.c"
    "${SDK_PATH}/security_tfm/platform/ext/target/siliconlabs/hse/sli_se.c"
)

target_include_directories(slc PUBLIC
   "../config"
   "../autogen"
    "${SDK_PATH}/devices/platform/Device/SiliconLabs/EFR32MG24/Include"
    "${SDK_PATH}/platform_common/platform/common/inc"
    "${SDK_PATH}/bootloader/platform/bootloader"
    "${SDK_PATH}/bootloader/platform/bootloader/api"
    "${SDK_PATH}/bootloader/platform/bootloader/parser/compression"
    "${SDK_PATH}/bootloader/platform/bootloader/debug"
    "${SDK_PATH}/bootloader/platform/bootloader/parser"
    "${SDK_PATH}/bootloader/platform/bootloader/core/flash"
    "${SDK_PATH}/bootloader/platform/bootloader/security"
    "${SDK_PATH}/cmsis/Core/Include"
    "${SDK_PATH}/platform_core/platform/service/device_init/inc"
    "${SDK_PATH}/platform_core/platform/emlib/inc"
    "${SDK_PATH}/platform_core/platform/common/errno_error_codes/inc"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_mbedtls_support/config"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_mbedtls_support/config/preset"
    "${SDK_PATH}/security_mbedtls_source/include"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_mbedtls_support/inc"
    "${SDK_PATH}/security_mbedtls_source/library"
    "${SDK_PATH}/platform_core/platform/service/memory_manager/inc"
    "${SDK_PATH}/platform_core/platform/service/memory_manager/src"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/inc"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/inc"
    "${SDK_PATH}/platform_core/platform/common/inc"
    "${SDK_PATH}/security_tfm/lib/fih/inc"
    "${SDK_PATH}/security_tfm/platform/include"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/sli_psec_osal/inc"
)

target_compile_definitions(slc PUBLIC
    "EFR32MG24B010F1536IM40=1"
    "SL_CODE_COMPONENT_SYSTEM=system"
    "SE_MANAGER_CONFIG_FILE=\"btl_aes_ctr_stream_block_cfg.h\""
    "BTL_PARSER_SUPPORT_CUSTOM_TAGS=1"
    "BTL_PARSER_SUPPORT_LZMA=1"
    "_LZMA_SIZE_OPT=1"
    "BOOTLOADER_ENABLE=1"
    "BOOTLOADER_SECOND_STAGE=1"
    "SL_RAMFUNC_DISABLE=1"
    "__START=main"
    "__STARTUP_CLEAR_BSS=1"
    "SYSTEM_NO_STATIC_MEMORY=1"
    "BTL_PARSER_SUPPORT_LZ4=1"
    "BOOTLOADER_SUPPORT_INTERNAL_STORAGE=1"
    "BOOTLOADER_SUPPORT_STORAGE=1"
    "SL_COMPONENT_CATALOG_PRESENT=1"
    "MBEDTLS_CONFIG_FILE=<sl_mbedtls_trustzone_config.h>"
    "SL_CODE_COMPONENT_MEMORY_MANAGER=memory_manager"
    "MBEDTLS_PSA_CRYPTO_CONFIG_FILE=<psa_crypto_config.h>"
    "SL_CODE_COMPONENT_SE_MANAGER=se_manager"
    "SL_CODE_COMPONENT_CORE=core"
    "SL_CODE_COMPONENT_PSEC_OSAL=psec_osal"
    "SL_TRUSTZONE_SECURE=1"
)

target_link_libraries(slc PUBLIC
    "-Wl,--start-group"
    "gcc"
    "c"
    "m"
    "nosys"
    "-Wl,--end-group"
)
target_compile_options(slc PUBLIC
    $<$<COMPILE_LANGUAGE:C>:-mcpu=cortex-m33>
    $<$<COMPILE_LANGUAGE:C>:-mthumb>
    $<$<COMPILE_LANGUAGE:C>:-mfpu=fpv5-sp-d16>
    $<$<COMPILE_LANGUAGE:C>:-mfloat-abi=hard>
    $<$<COMPILE_LANGUAGE:C>:-mcmse>
    $<$<COMPILE_LANGUAGE:C>:-Wall>
    $<$<COMPILE_LANGUAGE:C>:-Wextra>
    $<$<COMPILE_LANGUAGE:C>:-Os>
    $<$<COMPILE_LANGUAGE:C>:-fdata-sections>
    $<$<COMPILE_LANGUAGE:C>:-ffunction-sections>
    $<$<COMPILE_LANGUAGE:C>:-fomit-frame-pointer>
    $<$<COMPILE_LANGUAGE:C>:-g>
    $<$<COMPILE_LANGUAGE:C>:--specs=nano.specs>
    $<$<COMPILE_LANGUAGE:C>:-Wno-ignored-qualifiers>
    $<$<COMPILE_LANGUAGE:C>:-Wno-sign-compare>
    $<$<COMPILE_LANGUAGE:C>:-fno-lto>
    $<$<COMPILE_LANGUAGE:CXX>:-mcpu=cortex-m33>
    $<$<COMPILE_LANGUAGE:CXX>:-mthumb>
    $<$<COMPILE_LANGUAGE:CXX>:-mfpu=fpv5-sp-d16>
    $<$<COMPILE_LANGUAGE:CXX>:-mfloat-abi=hard>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-mcmse>
    $<$<COMPILE_LANGUAGE:CXX>:-Wall>
    $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
    $<$<COMPILE_LANGUAGE:CXX>:-Os>
    $<$<COMPILE_LANGUAGE:CXX>:-fdata-sections>
    $<$<COMPILE_LANGUAGE:CXX>:-ffunction-sections>
    $<$<COMPILE_LANGUAGE:CXX>:-fomit-frame-pointer>
    $<$<COMPILE_LANGUAGE:CXX>:-g>
    $<$<COMPILE_LANGUAGE:CXX>:--specs=nano.specs>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-ignored-qualifiers>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-sign-compare>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-lto>
    $<$<COMPILE_LANGUAGE:ASM>:-mcpu=cortex-m33>
    $<$<COMPILE_LANGUAGE:ASM>:-mthumb>
    $<$<COMPILE_LANGUAGE:ASM>:-mfpu=fpv5-sp-d16>
    $<$<COMPILE_LANGUAGE:ASM>:-mfloat-abi=hard>
    "$<$<COMPILE_LANGUAGE:ASM>:SHELL:-x assembler-with-cpp>"
)

set(post_build_command ${POST_BUILD_EXE} postbuild "./Matter-Bootloader.slpb" --parameter build_dir:"$<TARGET_FILE_DIR:Matter-Bootloader>")
set_property(TARGET slc PROPERTY C_STANDARD 17)
set_property(TARGET slc PROPERTY CXX_STANDARD 17)
set_property(TARGET slc PROPERTY CXX_EXTENSIONS OFF)

target_link_options(slc INTERFACE
    -mcpu=cortex-m33
    -mthumb
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
    -T${CMAKE_CURRENT_LIST_DIR}/../autogen/linkerfile.ld
    --specs=nano.specs
    "SHELL:-Xlinker -Map=$<TARGET_FILE_DIR:Matter-Bootloader>/Matter-Bootloader.map"
    -Wl,--no-warn-rwx-segment
    "SHELL:-Wl,--wrap=_free_r -Wl,--wrap=_malloc_r -Wl,--wrap=_calloc_r -Wl,--wrap=_realloc_r"
    -fno-lto
    -Wl,--gc-sections
)

# BEGIN_SIMPLICITY_STUDIO_METADATA=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=END_SIMPLICITY_STUDIO_METADATA