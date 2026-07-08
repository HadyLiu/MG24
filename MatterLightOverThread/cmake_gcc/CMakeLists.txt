# Define minimal required version of CMake.
cmake_minimum_required(VERSION "3.25")

# Project definition
project(
	MatterLightOverThread
	VERSION 1.0
	LANGUAGES C CXX ASM
)

# Include the definition of the slc_MatterLightOverThread target,
# which contains the content of the SLC project
include(MatterLightOverThread.cmake)

set(APP_LAYER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/hal/src/HalGpio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/hal/src/HalExti.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/hal/src/HalIadc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/hal/src/HalNvmstorage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/hal/src/HalPwm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/hal/src/HalSpiSm15135e.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/bsp/src/BspLedIndicatorRed.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/bsp/src/BspLedIndicatorWhite.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/bsp/src/BspLedWrgb.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/bsp/src/BspPowerMonitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/bsp/src/BspSleepTimer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/bsp/src/BspUartLog.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/middlewares/src/MatterBridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/middlewares/src/ButtonInput.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/middlewares/src/LightEffectProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/middlewares/src/LightEffectEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/middlewares/src/IndicatorEffectEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/middlewares/src/LightSequenceScheduler.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/service/src/ColorConverter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/service/src/ButtonService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/service/src/MatterBridgeServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/service/src/PowerServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/service/src/LightDecisionCenter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/service/src/LightNvmStorage.cpp

)

add_executable(MatterLightOverThread
    ${APP_LAYER_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/entry.cpp
)


target_include_directories(MatterLightOverThread PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/hal/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/bsp/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/service/inc      
    ${CMAKE_CURRENT_SOURCE_DIR}/../../User/middlewares/inc
   # ${CMAKE_CURRENT_SOURCE_DIR}/../policy

)

target_compile_definitions(MatterLightOverThread PUBLIC
    # Add additional macros here
)

target_compile_options(MatterLightOverThread PUBLIC
    # Set additional compiler flags here
)

target_link_options(MatterLightOverThread PUBLIC
    # Set additional linker flags here
)

# Link with the content defined in the SLC project
target_link_libraries(MatterLightOverThread PRIVATE
    slc
)

# Include managed project content if available
include(MatterLightOverThread_project.cmake OPTIONAL RESULT_VARIABLE managed_project)
if(managed_project)
    message(STATUS "Using managed project content from ${managed_project}")
endif()

# Force the gcc linker command
set_target_properties(MatterLightOverThread PROPERTIES LINKER_LANGUAGE C)

# Create .bin, .hex and .s37 artifacts after building the project
add_custom_command(TARGET MatterLightOverThread
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_SREC_CMD} "$<TARGET_FILE:MatterLightOverThread>" "$<TARGET_FILE_DIR:MatterLightOverThread>/$<TARGET_FILE_BASE_NAME:MatterLightOverThread>.s37"
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_IHEX_CMD} "$<TARGET_FILE:MatterLightOverThread>" "$<TARGET_FILE_DIR:MatterLightOverThread>/$<TARGET_FILE_BASE_NAME:MatterLightOverThread>.hex"
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_BIN_CMD}  "$<TARGET_FILE:MatterLightOverThread>" "$<TARGET_FILE_DIR:MatterLightOverThread>/$<TARGET_FILE_BASE_NAME:MatterLightOverThread>.bin" 
)

# Run post-build pipeline to perform additional post-processing
if(post_build_command)
add_custom_command(TARGET MatterLightOverThread
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
    COMMAND ${post_build_command}
)
endif()
