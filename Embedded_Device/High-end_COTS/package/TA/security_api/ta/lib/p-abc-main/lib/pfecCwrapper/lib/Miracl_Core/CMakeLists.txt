cmake_minimum_required(VERSION 3.5)

project(m_core VERSION 1.0.0 DESCRIPTION "Compiled 64 bits from Miracl/core library")

add_library(
    ${PROJECT_NAME} 
        STATIC 
        IMPORTED GLOBAL
    )

set_target_properties(
${PROJECT_NAME} 
    PROPERTIES 
    IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/core.a"
    )

target_include_directories(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_LIST_DIR}")
