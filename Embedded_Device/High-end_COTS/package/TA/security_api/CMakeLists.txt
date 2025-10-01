project (security_api C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set (ACEUNIT_PATH host/lib/aceunit)
set (CRC_PATH host/lib/libcrc)
set (TEEC_PATH ../../../../../optee_client/libteec)

set (SRC host/main.c
         host/custom_se_pkcs11.c
         host/rfc4764.c
         host/network_manager.c
         host/testcases.c
         host/bootstrapping_agent.c
         ${CRC_PATH}/src/crc16.c)

set (SRC_AGENT  host/custom_se_pkcs11.c
                host/rfc4764.c
                host/network_manager.c
                host/bootstrapping_agent.c
                host/ERA_agent.c
                host/security_api_interface.c
                ${CRC_PATH}/src/crc16.c)

set (TEST_NAME ${PROJECT_NAME}_test)

set (AGENT_NAME ${PROJECT_NAME}_interface)

# add_subdirectory (ta/lib/p-abc-main)

add_custom_command(OUTPUT host/testcases.c
                   COMMAND make TEEC_EXPORT=${TEEC_PATH}
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/host
                   DEPENDS host/main.c
)

add_compile_definitions(_GNU_SOURCE)
add_executable (${TEST_NAME} ${SRC})
add_executable (${AGENT_NAME} ${SRC_AGENT})
add_library (${PROJECT_NAME} STATIC host/custom_se_pkcs11.c
                                    host/bootstrapping_agent.c
                                    host/rfc4764.c)

include_directories( 
        PRIVATE ta/include
        PRIVATE host/include
        PRIVATE ${ACEUNIT_PATH}/include
        PRIVATE ${CRC_PATH}/include
        # PRIVATE ta/lib/p-abc-main/include
        # PRIVATE ta/lib/p-abc-main/lib/pfecCwrapper/include
)


target_link_libraries (${PROJECT_NAME} PRIVATE teec)
target_link_libraries (${TEST_NAME} PRIVATE teec)
target_link_libraries (${AGENT_NAME} PRIVATE teec)
# target_link_libraries (${PROJECT_NAME} PRIVATE dpabc_psms)
# target_link_libraries (${TEST_NAME} PRIVATE dpabc_psms)

add_library(aceunit STATIC IMPORTED)
set_target_properties(aceunit PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/${ACEUNIT_PATH}/lib/libaceunit-setjmp.a)
target_link_libraries (${TEST_NAME} PRIVATE aceunit)
# target_link_libraries (${AGENT_NAME} PRIVATE aceunit)

# SET(BUNDLED_NAME "dpabc_psms_middleware_bundled")
# bundle_static_library(${PROJECT_NAME} ${BUNDLED_NAME})

install (TARGETS ${TEST_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
install (TARGETS ${AGENT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
