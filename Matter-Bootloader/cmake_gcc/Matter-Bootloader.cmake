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