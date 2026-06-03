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