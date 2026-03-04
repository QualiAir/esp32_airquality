# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

idf_component_register(SRCS ${app_sources} INCLUDE_DIRS "." REQUIRES esp_adc driver hal)
