# Define minimal required version of CMake.
cmake_minimum_required(VERSION "3.25")

# Project definition
project(Li-Bat_MatterLight_EFR32MG24B010F1536IM40)

include(ExternalProject)

if(NOT DEFINED LI_BAT_BUILD_RELEASE)
    if(DEFINED ENV{LI_BAT_BUILD_VARIANT}
        AND "$ENV{LI_BAT_BUILD_VARIANT}" STREQUAL "release")
        set(LI_BAT_BUILD_RELEASE ON)
    else()
        set(LI_BAT_BUILD_RELEASE OFF)
    endif()
endif()
message(STATUS "LI_BAT_BUILD_RELEASE=${LI_BAT_BUILD_RELEASE}")

ExternalProject_Add(MatterLightOverThread
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../MatterLightOverThread/cmake_gcc
  BINARY_DIR ${CMAKE_CURRENT_LIST_DIR}/../MatterLightOverThread/cmake_gcc/build
  CMAKE_ARGS --preset project -DLI_BAT_BUILD_RELEASE=${LI_BAT_BUILD_RELEASE}
  BUILD_COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
  INSTALL_COMMAND ""
  BUILD_ALWAYS true
  USES_TERMINAL_BUILD 1
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../MatterLightOverThread/cmake_gcc
)
ExternalProject_Add(Matter-Bootloader
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../Matter-Bootloader/cmake_gcc
  BINARY_DIR ${CMAKE_CURRENT_LIST_DIR}/../Matter-Bootloader/cmake_gcc/build
  CMAKE_ARGS --preset project
  BUILD_COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIG>
  INSTALL_COMMAND ""
  BUILD_ALWAYS true
  USES_TERMINAL_BUILD 1
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../Matter-Bootloader/cmake_gcc
)

ExternalProject_Add_StepDependencies(Matter-Bootloader build MatterLightOverThread)


set(post_build_command ${POST_BUILD_EXE} postbuild "./Li-Bat_MatterLight_EFR32MG24B010F1536IM40.slpb")


add_custom_target(postbuild ALL
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
    COMMAND ${post_build_command}
)

add_dependencies(postbuild MatterLightOverThread Matter-Bootloader)

# BEGIN_SIMPLICITY_STUDIO_METADATA=eJy9U11P2zAU/StRhCZ4aG7aMh5KNrRSmCIlZMJF2qRIkWubxptjZ7ZDQNP++y6UpkUb2p72eK/PuefcD/8ISZp/ytLzdPmlIsubRVpUpMhulmlxVeXF4iILZ2Fydt+o4E5YJ41+V4bjKC7DQGhmuNRrTHwg52lahmfvS53kxKjOI9AFyNJudu8kQmrv2xlA3/dRP42MXcMkjsfwOc8Iq0VDR1I7TzUTWBkZM22uaCNcS5nYADLDqN/o79VyUtGVi5hpwDnYSkeCGYuV0E8QJO45+xRhvEXlhndKXItbYbEX4QLJsbhTLOqN/fYkHXHqaRnCQF3kC3ErtXzkv45/hiOhNc6vOqk48aId0viwSx4covuGai7sUTDggzffO+NPDw637o8gk6M59VVOvRc2k+vaVxeX19NJ/nFyPI/H8eX47fQkzY/jyKl2teEn8Lt6An92lbTWfBXMBxonj53tCRW4/GVtBeW4HrXbBNSmEVBT/qBkB0Q2rZJM+gfiOy4N3J1Uw2j+3T68Igx/cTqaG+OVoTjI/+fyhehwJ/DiUDZnCHt3mMDum2AY/vwFnjMzvQ===END_SIMPLICITY_STUDIO_METADATA