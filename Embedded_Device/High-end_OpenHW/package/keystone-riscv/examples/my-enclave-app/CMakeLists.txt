# Define nombres de binarios y archivos fuente
set(eapp_bin my-enclave-app)
set(eapp_src eapp/main_eapp.c tiny_aes_c/aes.c tiny_aes_c/aes-cbc-cmac.c tiny_aes_c/TI_aes_128.c crypto_utils/crypto_utils.c tiny_aes_c/aes256.c tiny_sha256/sha-256.c eapp/ocall_handler_eapp.c eapp/crypto_enclave.c)
set(host_bin my-enclave-host)
set(host_src host/main.cpp host/ocall_handler.cpp host/edgecall_handler.cpp host/network_manager.cpp host/rfc4764.cpp host/bootstrapping_agent.cpp host/ERA_agent.cpp host/security_api_interface.cpp host/csp_api.cpp)
set(package_name "my-enclave.ke")
set(package_script "./my-enclave-host my-enclave-app eyrie-rt loader.bin")
set(eyrie_plugins "none")
set(tiny_aes "/home/jotace/ANTs/keystone-riscv/examples/my-enclave-app/tiny_aes_c/aes.c")
set(tiny_aes256 "/home/jotace/ANTs/keystone-riscv/examples/my-enclave-app/tiny_aes_c/aes256.c")
set(aes-cbc-cmac "/home/jotace/ANTs/keystone-riscv/examples/my-enclave-app/tiny_aes_c/aes-cbc-cmac.c")
set(TI_aes_128 "/home/jotace/ANTs/keystone-riscv/examples/my-enclave-app/tiny_aes_c/TI_aes_128.c")

# Include directory
include_directories("/home/jotace/ANTs/keystone-riscv/examples/my-enclave-app/tiny_aes_c" "${CMAKE_CURRENT_SOURCE_DIR}/common" "${CMAKE_CURRENT_SOURCE_DIR}/host/include" "${CMAKE_CURRENT_SOURCE_DIR}/crypto_utils" "${CMAKE_CURRENT_SOURCE_DIR}/eapp/include" "${CMAKE_CURRENT_SOURCE_DIR}/tiny_sha256")

# Compilación del enclave (eapp)
add_executable(${eapp_bin} ${eapp_src})
target_link_libraries(${eapp_bin} "-nostdlib -static" ${KEYSTONE_LIB_EAPP} ${KEYSTONE_LIB_EDGE})
target_include_directories(${eapp_bin}
  PUBLIC ${KEYSTONE_SDK_DIR}/include/app
  PUBLIC ${KEYSTONE_SDK_DIR}/include/edge)

set_target_properties(${eapp_bin}
  PROPERTIES LINK_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/eapp/app.lds")

# Compilación del host
add_executable(${host_bin} ${host_src})
target_link_libraries(${host_bin} ${KEYSTONE_LIB_HOST} ${KEYSTONE_LIB_EDGE})
set_target_properties(${host_bin}
  PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
target_include_directories(${host_bin}
  PUBLIC ${KEYSTONE_SDK_DIR}/include/host
  PUBLIC ${KEYSTONE_SDK_DIR}/include/edge)

# Crear runtime Eyrie asociado
set(eyrie_files_to_copy .options_log eyrie-rt loader.bin)
add_eyrie_runtime(${eapp_bin}-eyrie
  ${eyrie_plugins}
  ${eyrie_files_to_copy})

# Empaquetado Keystone (.ke)
add_keystone_package(${eapp_bin}-package
  ${package_name}
  ${package_script}
  ${eyrie_files_to_copy} ${eapp_bin} ${host_bin})
add_dependencies(${eapp_bin}-package ${eapp_bin}-eyrie)

# Agregar al target principal
add_dependencies(examples ${eapp_bin}-package)
