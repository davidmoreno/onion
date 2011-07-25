include_directories (${CMAKE_SOURCE_DIR}/src/) 
include_directories (${INCLUDE_PATH}/libxml2/) 

add_executable(webdav webdav.c main.c)
target_link_libraries(webdav onion_static ${XML2_LIB})

