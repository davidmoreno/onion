include_directories (${CMAKE_SOURCE_DIR}/src/onion/ ${CMAKE_SOURCE_DIR}/src/onionssl/) 


add_library(onion_ssl onion_ssl.c)

target_link_libraries(onion_ssl)

target_link_libraries(onion_ssl gnutls gcrypt)

