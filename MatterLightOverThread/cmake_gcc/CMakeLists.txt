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

add_executable(MatterLightOverThread
    # Add additional sources here
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/my_app.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../driver/iadc_driver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../driver/button.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../driver/led_pwm_port.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../driver/led_driver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../driver/pinManage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../driver/powerManage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../driver/sm15135e.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../driver/matter_interfacing.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../driver/time.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../driver/main_light_control.cpp
    

    ${CMAKE_CURRENT_SOURCE_DIR}/../test/test_adc.cpp # 测试用例
)

target_include_directories(MatterLightOverThread PUBLIC
    # Add additional include paths here
    #${CMAKE_CURRENT_SOURCE_DIR}/../app
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