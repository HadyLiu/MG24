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

# BEGIN_SIMPLICITY_STUDIO_METADATA=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END_SIMPLICITY_STUDIO_METADATA