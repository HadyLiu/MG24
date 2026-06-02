# Define minimal required version of CMake.
cmake_minimum_required(VERSION "3.25")

# Project definition
project(Li-Bat_MatterLight_EFR32MG24B010F1536IM40)

include(ExternalProject)

ExternalProject_Add(MatterLightOverThread
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../MatterLightOverThread/cmake_gcc
  BINARY_DIR ${CMAKE_CURRENT_LIST_DIR}/../MatterLightOverThread/cmake_gcc/build
  CMAKE_ARGS --preset project
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

# BEGIN_SIMPLICITY_STUDIO_METADATA=eJy9U11P2zAU/StWhCZ4aG7aMh5KNrRSmCIlZCJF2qRIkWubxptjZ7ZDQNP++y6Upt0H2p72eK/PuefcD38LiiT7kCbnyfJTVSxvFkleFXl6s0zyqyrLFxdpMAvis/tGkTthnTT6TRmMw6gMiNDMcKnXmHhXnCdJGZy9LXWcFUZ1HoGOIEu72b2TCKm9b2cAfd+H/TQ0dg2TKBrDxywtWC0aOpLaeaqZwMrImGlzRRvhWsrEBpAaRv1Gf6+Wk4quXMhMA87BVjoUzFishH4Iid1z9inCeIvKDO+UuBa3wmIvwhHJsbhTLOyN/fIkHXLqaRnAQF1kC3ErtXzkv4x/hiOhNc6vOql44UU7pPFhlzw4RPcN1VzYIzLgyauvnfGnB4db90eQytGc+iqj3gubynXtq4vL6+kkez85nkfj6HL8enqSZMdR6FS72vBj+F09hj+7iltrPgvmicbJY2d7Qjkuf1lbQXk1wQWp3S6gNo2AmvIHJTsoZNMqyaR/KHzHpYG7k2oYzr83AC9Kw1/cjubGeGUoDvN/Ov1FdrgX+OlgNucIe/cYw+67YBh8/wHo5DYB=END_SIMPLICITY_STUDIO_METADATA