include_directories (${CMAKE_SOURCE_DIR}/src/onion/ ${CMAKE_SOURCE_DIR}/src/onionssl/) 


add_library(onionssl onionssl.c)

target_link_libraries(onionssl)

target_link_libraries(onionssl gnutls)

