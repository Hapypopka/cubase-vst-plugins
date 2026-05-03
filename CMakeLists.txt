cmake_minimum_required(VERSION 3.22)
project(CubaseVSTPlugins VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# JUCE подтягивается через GitHub Actions cache, либо лежит локально рядом
if(NOT DEFINED JUCE_DIR)
    set(JUCE_DIR ${CMAKE_SOURCE_DIR}/JUCE)
endif()

if(NOT EXISTS ${JUCE_DIR}/CMakeLists.txt)
    message(FATAL_ERROR "JUCE not found at ${JUCE_DIR}. Clone it: git clone --depth 1 --branch 7.0.12 https://github.com/juce-framework/JUCE.git")
endif()

add_subdirectory(${JUCE_DIR} JUCE)

# Все плагины из plugins/* подключаются автоматически
file(GLOB PLUGIN_DIRS RELATIVE ${CMAKE_SOURCE_DIR}/plugins ${CMAKE_SOURCE_DIR}/plugins/*)
foreach(plugin ${PLUGIN_DIRS})
    set(PLUGIN_PATH ${CMAKE_SOURCE_DIR}/plugins/${plugin})
    if(IS_DIRECTORY ${PLUGIN_PATH} AND EXISTS ${PLUGIN_PATH}/CMakeLists.txt)
        message(STATUS "Adding plugin: ${plugin}")
        add_subdirectory(plugins/${plugin})
    endif()
endforeach()
