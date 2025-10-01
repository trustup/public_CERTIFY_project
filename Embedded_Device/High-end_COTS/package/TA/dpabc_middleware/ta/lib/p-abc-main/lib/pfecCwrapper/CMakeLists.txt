cmake_minimum_required(VERSION 3.11)

project(pfeclib VERSION 0.0.1 DESCRIPTION "Wrapper library for pairing-friendly EC arithmetic")

# C11
set(CMAKE_C_STANDARD 11)

# Fix for sh.exe in path
set(DCMAKE_SH="CMAKE_SH-NOTFOUND")

#Fix for cmocka in Windows (need to adjust path to your installation)
if(WIN32 AND NOT CMOCKA_PATH)
    set(CMOCKA_PATH "C:/msys64/var/local/cmocka/cmocka.dll")
endif(WIN32 AND NOT CMOCKA_PATH)

# Set folder paths
SET(SRC_PATH_WRAPPER "${PROJECT_SOURCE_DIR}/src")
SET(HEADER_PATH_WRAPPER "${PROJECT_SOURCE_DIR}/include")
SET(LIB_PATH_WRAPPER "${PROJECT_SOURCE_DIR}/lib")
SET(TEST_PATH_WRAPPER "${PROJECT_SOURCE_DIR}/test")

# Add librarie's subdirectories
add_subdirectory("${LIB_PATH_WRAPPER}/Miracl_Core")

# Includes (must be after add_subdirectories for libraries. Alternatively, use target_include_directories)
include_directories(${HEADER_PATH_WRAPPER})


if(${WRAPPER_INSTANTIATION} STREQUAL "pfec_Miracl_Bls381_32")
        # Miracle Core BLS381_32bits instantiation
        SET(M_BLS381_32 "pfec_Miracl_Bls381_32")

        SET(M_BLS381_32_SRC_PATH "${SRC_PATH_WRAPPER}/Miracl_BLS12381_32b")

        add_library(${M_BLS381_32} STATIC 
                "${M_BLS381_32_SRC_PATH}/types.h"
                "${M_BLS381_32_SRC_PATH}/Zp.c"
                "${M_BLS381_32_SRC_PATH}/g1.c"
                "${M_BLS381_32_SRC_PATH}/g2.c"
                "${M_BLS381_32_SRC_PATH}/g3.c"
                "${M_BLS381_32_SRC_PATH}/pair.c"
                "${M_BLS381_32_SRC_PATH}/utils.c")

        target_include_directories(${M_BLS381_32} PUBLIC ${HEADER_PATH_WRAPPER})

        target_link_libraries(${M_BLS381_32}
                m_core)
        
        set_target_properties(${M_BLS381_32}
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output/lib/pfec_Miracl_Bls381_32"
        LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output/lib/pfec_Miracl_Bls381_32"
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output/bin/pfec_Miracl_Bls381_32"
        )

endif()


if(${WRAPPER_INSTANTIATION} STREQUAL "pfec_Miracl_Bls381_64")
        # Miracle Core BLS381_64bits instantiation
        SET(M_BLS381_64 "pfec_Miracl_Bls381_64")

        SET(M_BLS381_64_SRC_PATH "${SRC_PATH_WRAPPER}/Miracl_BLS12381_64b")

        add_library(${M_BLS381_64} STATIC 
                "${M_BLS381_64_SRC_PATH}/types.h"
                "${M_BLS381_64_SRC_PATH}/Zp.c"
                "${M_BLS381_64_SRC_PATH}/g1.c"
                "${M_BLS381_64_SRC_PATH}/g2.c"
                "${M_BLS381_64_SRC_PATH}/g3.c"
                "${M_BLS381_64_SRC_PATH}/pair.c"
                "${M_BLS381_64_SRC_PATH}/utils.c")

        target_include_directories(${M_BLS381_64} PUBLIC ${HEADER_PATH_WRAPPER})

        target_link_libraries(${M_BLS381_64}
                m_core)

        set_target_properties(${M_BLS381_64}
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output/lib/pfec_Miracl_Bls381_64"
        LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output/lib/pfec_Miracl_Bls381_64"
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output/bin/pfec_Miracl_Bls381_64"
        )

endif()

#[[
# Example executable
#add_executable(example_test 
#        "${SRC_PATH_WRAPPER}/example/main_test.c")

#target_link_libraries(example_test
#        ${M_BLS381_64})


# Fix for cmocka in windows
if(WIN32)
        file(COPY ${CMOCKA_PATH} DESTINATION ${PROJECT_BINARY_DIR})
endif(WIN32)

]]