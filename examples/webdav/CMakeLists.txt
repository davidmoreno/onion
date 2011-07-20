include_directories (${CMAKE_SOURCE_DIR}/src/) 
include_directories (/usr/include/libxml2/) 

add_executable(webdav webdav.c main.c)
target_link_libraries(webdav onion_static xml2)

