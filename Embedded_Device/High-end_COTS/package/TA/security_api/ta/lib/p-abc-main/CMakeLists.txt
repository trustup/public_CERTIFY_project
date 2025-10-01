cmake_minimum_required(VERSION 3.11)

project(dpabc_psms VERSION 0.0.1 DESCRIPTION "Iplementation of dP-ABC (PS-MS) in C, using wrapper library")

# C11
set(CMAKE_C_STANDARD 11)

# Fix for sh.exe in path
set(DCMAKE_SH="CMAKE_SH-NOTFOUND")

# Set default value for wrapper library instantiation 
if(NOT WRAPPER_INSTANTIATION)
    set(WRAPPER_INSTANTIATION "pfec_Miracl_Bls381_64")
endif()

# Function for bundling static libraries for convenience 
SET(BUNDLED_NAME "dpabc_psms_bundled")
function(bundle_static_library tgt_name bundled_tgt_name)
  list(APPEND static_libs ${tgt_name})

  function(_recursively_collect_dependencies input_target)
    set(_input_link_libraries LINK_LIBRARIES)
    get_target_property(_input_type ${input_target} TYPE)
    if (${_input_type} STREQUAL "INTERFACE_LIBRARY")
      set(_input_link_libraries INTERFACE_LINK_LIBRARIES)
    endif()
    get_target_property(public_dependencies ${input_target} ${_input_link_libraries})
    foreach(dependency IN LISTS public_dependencies)
      if(TARGET ${dependency})
        get_target_property(alias ${dependency} ALIASED_TARGET)
        if (TARGET ${alias})
          set(dependency ${alias})
        endif()
        get_target_property(_type ${dependency} TYPE)
        if (${_type} STREQUAL "STATIC_LIBRARY")
          list(APPEND static_libs ${dependency})
        endif()

        get_property(library_already_added
          GLOBAL PROPERTY _${tgt_name}_static_bundle_${dependency})
        if (NOT library_already_added)
          set_property(GLOBAL PROPERTY _${tgt_name}_static_bundle_${dependency} ON)
          _recursively_collect_dependencies(${dependency})
        endif()
      endif()
    endforeach()
    set(static_libs ${static_libs} PARENT_SCOPE)
  endfunction()

  _recursively_collect_dependencies(${tgt_name})

  list(REMOVE_DUPLICATES static_libs)

  set(bundled_tgt_full_name 
    ${CMAKE_CURRENT_LIST_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${bundled_tgt_name}${CMAKE_STATIC_LIBRARY_SUFFIX})

  # if (CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|GNU)$")
    file(WRITE ${CMAKE_CURRENT_LIST_DIR}/${bundled_tgt_name}.ar.in
      "CREATE ${bundled_tgt_full_name}\n" )
        
    foreach(tgt IN LISTS static_libs)
      file(APPEND ${CMAKE_CURRENT_LIST_DIR}/${bundled_tgt_name}.ar.in
        "ADDLIB $<TARGET_FILE:${tgt}>\n")
    endforeach()
    
    file(APPEND ${CMAKE_CURRENT_LIST_DIR}/${bundled_tgt_name}.ar.in "SAVE\n")
    file(APPEND ${CMAKE_CURRENT_LIST_DIR}/${bundled_tgt_name}.ar.in "END\n")

    file(GENERATE
      OUTPUT ${CMAKE_CURRENT_LIST_DIR}/${bundled_tgt_name}.ar
      INPUT ${CMAKE_CURRENT_LIST_DIR}/${bundled_tgt_name}.ar.in)

    set(ar_tool ${CMAKE_CXX_COMPILER_AR})

    add_custom_command(
      COMMAND ${ar_tool} -M < ${CMAKE_CURRENT_LIST_DIR}/${bundled_tgt_name}.ar
      OUTPUT ${bundled_tgt_full_name}
      COMMENT "Bundling ${bundled_tgt_name}"
      DEPENDS ${static_libs}
      VERBATIM)
  # elseif(MSVC)
  #   find_program(lib_tool lib)
  #
  #   foreach(tgt IN LISTS static_libs)
  #     list(APPEND static_libs_full_names $<TARGET_FILE:${tgt}>)
  #   endforeach()
  #
  #   add_custom_command(
  #     COMMAND ${lib_tool} /NOLOGO /OUT:${bundled_tgt_full_name} ${static_libs_full_names}
  #     OUTPUT ${bundled_tgt_full_name}
  #     COMMENT "Bundling ${bundled_tgt_name}"
  #     DEPENDS ${static_libs}
  #     VERBATIM)
  # elseif(APPLE)
  #   foreach(tgt IN LISTS static_libs)
  #     list(APPEND static_libs_full_names $<TARGET_FILE:${tgt}>)
  #   endforeach()
  #
  #   add_custom_command(
  #     COMMAND libtool -static -o ${bundled_tgt_full_name} ${static_libs_full_names}
  #     OUTPUT ${bundled_tgt_full_name}
  #     COMMENT "Bundling ${bundled_tgt_name}"
  #     DEPENDS ${static_libs}
  #     VERBATIM)
  # else()
  #   message(FATAL_ERROR "Unknown bundle scenario!")
  # endif()
  #
  add_custom_target(bundling_target ALL DEPENDS ${bundled_tgt_full_name})
  add_dependencies(bundling_target ${tgt_name})

  add_library(${bundled_tgt_name} STATIC IMPORTED)
  set_target_properties(${bundled_tgt_name} 
    PROPERTIES 
      IMPORTED_LOCATION ${bundled_tgt_full_name}
      INTERFACE_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${tgt_name},INTERFACE_INCLUDE_DIRECTORIES>)
  add_dependencies(${bundled_tgt_name} bundling_target)

endfunction()

# Set folder paths
SET(SRC_PATH_PABC "${PROJECT_SOURCE_DIR}/src")
SET(HEADER_PATH_PABC "${PROJECT_SOURCE_DIR}/include")
SET(LIB_PATH_PABC "${PROJECT_SOURCE_DIR}/lib")
SET(TEST_PATH_PABC "${PROJECT_SOURCE_DIR}/test")

# Add libraries' subdirectories
add_subdirectory("${LIB_PATH_PABC}/pfecCwrapper")

# Includes (must be after add_subdirectories for libraries. Alternatively, use target_include_directories)
include_directories(${HEADER_PATH_PABC})

#Library with dpabc functionality, wrapper instantiation established by user (or default)
add_library(dpabc_psms STATIC 
        "${SRC_PATH_PABC}/PSMS/Dpabc_utils.h"
        "${SRC_PATH_PABC}/PSMS/types_impl.h"
        "${SRC_PATH_PABC}/PSMS/Dpabc_utils.c"
        "${SRC_PATH_PABC}/PSMS/Dpabc_types.c"
        "${SRC_PATH_PABC}/PSMS/Dpabc.c"
        )

target_include_directories(dpabc_psms PUBLIC ${HEADER_PATH_PABC})

target_link_libraries(dpabc_psms ${WRAPPER_INSTANTIATION})

# Example binary
add_executable(dpabc_example 
        "${SRC_PATH_PABC}/example/main.c")
target_link_libraries(dpabc_example dpabc_psms)
                
# Bundled library generation
# bundle_static_library(dpabc_psms ${BUNDLED_NAME})
#
# # Modifying output folders for libs/binaries
# set_target_properties(dpabc_example dpabc_psms ${BUNDLED_NAME}
# PROPERTIES
# ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output/lib"
# LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output/lib"
# RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output/bin"
# )
