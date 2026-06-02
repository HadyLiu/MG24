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