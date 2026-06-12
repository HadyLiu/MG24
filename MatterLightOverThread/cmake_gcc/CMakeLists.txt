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
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/AppOrchestrator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../integration/MatterBridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../policy/BatteryPolicy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../policy/ButtonPolicy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../policy/CommissioningManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../policy/FactoryResetManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../policy/IdentifyEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../service/ButtonInput.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../service/ColorConverter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../service/ColorLibrary.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../service/IndicatorService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../service/LightEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../service/PowerMonitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../service/SystemTime.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../hal/NvmStorage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../hal/PwmHal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../hal/SpiRgbHal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../hal/SpiSm15135eHal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../hal/led_pwm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../hal/PinHal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../hal/IadcHal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../hal/IndicatorHal.cpp

)

add_executable(MatterLightOverThread
    ${APP_LAYER_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/MyCustomMonitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../test/test_adc.cpp
)


target_include_directories(MatterLightOverThread PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/app
    ${CMAKE_CURRENT_SOURCE_DIR}/../app
    ${CMAKE_CURRENT_SOURCE_DIR}/../integration
    ${CMAKE_CURRENT_SOURCE_DIR}/../policy
    ${CMAKE_CURRENT_SOURCE_DIR}/../service
    ${CMAKE_CURRENT_SOURCE_DIR}/../hal
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
