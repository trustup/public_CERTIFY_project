cmake_minimum_required(VERSION 3.5)

project(mcbls381_32b VERSION 1.0.0 DESCRIPTION "Compiled BLS12-381 32 bits curve from Miracl/core library")

add_library(
    ${PROJECT_NAME} 
        STATIC 
        IMPORTED GLOBAL
    )

    
if (WIN32)
    set_target_properties(
    ${PROJECT_NAME} 
        PROPERTIES 
        IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/core_win_alternative.a"
    )
endif (WIN32)


if (UNIX)
    set_target_properties(
    ${PROJECT_NAME} 
        PROPERTIES 
        IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/core_unix.a"
    )
endif (UNIX)




target_include_directories(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_LIST_DIR}")