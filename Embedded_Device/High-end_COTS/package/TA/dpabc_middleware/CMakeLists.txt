project (dpabc C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set (SRC host/main.c
         host/dpabc_middleware.c
         host/testcases.c)

set (SRC_GENERATE_ZKT host/dpabc_middleware.c
                      host/generateZkToken.c)
                
set (SRC_SETUP_SIG host/dpabc_middleware.c
                   host/setup_demo2.1.c)

set (SRC_VERIFY_SIG host/dpabc_middleware.c
                    host/verifySignature.c)


set (ACEUNIT_PATH host/lib/aceunit)

set (TEST_NAME ${PROJECT_NAME}_test)
set (SETUP_SIG_NAME signature_setup)
set (GENERATE_ZKT_NAME generate_zktoken)
set (VERIFY_SIG_NAME verify_signature)

set(WRAPPER_INSTANTIATION "pfec_Miracl_Bls381_64")
add_subdirectory (ta/lib/p-abc-main)

add_custom_command(OUTPUT host/testcases.c
                   COMMAND make TEEC_EXPORT=../../../../../optee_client/libteec
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/host
                   DEPENDS host/main.c
)

add_compile_definitions(_GNU_SOURCE)
add_executable (${TEST_NAME} ${SRC})
add_executable (${SETUP_SIG_NAME} ${SRC_SETUP_SIG})
add_executable (${GENERATE_ZKT_NAME} ${SRC_GENERATE_ZKT})
add_executable (${VERIFY_SIG_NAME} ${SRC_VERIFY_SIG})
add_library (${PROJECT_NAME} STATIC host/dpabc_middleware.c)

include_directories( 
        PRIVATE ta/include
        PRIVATE ta/lib/p-abc-main/include
        PRIVATE ta/lib/p-abc-main/lib/pfecCwrapper/include
        PRIVATE ta/lib/p-abc-main/lib/pfecCwrapper/lib/Miracl_Core
        PRIVATE host/include
        PRIVATE ${ACEUNIT_PATH}/include)


target_link_libraries (${PROJECT_NAME} PRIVATE teec)
target_link_libraries (${TEST_NAME} PRIVATE teec)
target_link_libraries (${SETUP_SIG_NAME} PRIVATE teec)
target_link_libraries (${GENERATE_ZKT_NAME} PRIVATE teec)
target_link_libraries (${VERIFY_SIG_NAME} PRIVATE teec)
target_link_libraries (${PROJECT_NAME} PRIVATE dpabc_psms)
target_link_libraries (${TEST_NAME} PRIVATE dpabc_psms)
target_link_libraries (${SETUP_SIG_NAME} PRIVATE dpabc_psms)
target_link_libraries (${GENERATE_ZKT_NAME} PRIVATE dpabc_psms)
target_link_libraries (${VERIFY_SIG_NAME} PRIVATE dpabc_psms)

add_library(aceunit STATIC IMPORTED)
set_target_properties(aceunit PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/${ACEUNIT_PATH}/lib/libaceunit-setjmp.a)
target_link_libraries (${TEST_NAME} PRIVATE aceunit)

SET(BUNDLED_NAME "dpabc_psms_middleware_bundled")
bundle_static_library(${PROJECT_NAME} ${BUNDLED_NAME})

install (TARGETS ${TEST_NAME} ${SETUP_SIG_NAME} ${GENERATE_ZKT_NAME} ${VERIFY_SIG_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
