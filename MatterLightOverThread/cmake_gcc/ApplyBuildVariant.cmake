# 构建变体：dev（默认可调日志）/ release（关调试日志与 Matter CLI）
# 由 LI_BAT_BUILD_RELEASE 注入；Studio Generate 后请保留 CMakeLists 里的 include。

if(NOT DEFINED LI_BAT_BUILD_RELEASE)
    set(LI_BAT_BUILD_RELEASE OFF)
endif()

if(LI_BAT_BUILD_RELEASE)
    message(STATUS "LI_BAT firmware variant: release")
    set(_li_bat_release_defs
        LI_BAT_BUILD_RELEASE=1
        APP_LOG_ENABLED=0
        SILABS_LOG_ENABLED=0
        SL_MATTER_CLI_ARG_PARSER=0
    )
    if(TARGET slc)
        target_compile_definitions(slc PUBLIC ${_li_bat_release_defs})
    endif()
    if(TARGET MatterLightOverThread)
        target_compile_definitions(MatterLightOverThread PUBLIC
            ${_li_bat_release_defs})
    endif()
else()
    message(STATUS "LI_BAT firmware variant: dev")
    if(TARGET MatterLightOverThread)
        target_compile_definitions(MatterLightOverThread PUBLIC
            LI_BAT_BUILD_RELEASE=0)
    endif()
endif()
