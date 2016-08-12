SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++0x")


include_directories (${PROJECT_SOURCE_DIR}/src/) 
include_directories (${PROJECT_SOURCE_DIR}/src/bindings/cpp/)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(01-basic 01-basic.cpp)
target_link_libraries(01-basic onion onioncpp)
 

add_executable(02-dict 02-dict.cpp)
target_link_libraries(02-dict onion onioncpp)
 
add_executable(03-urls 03-urls.cpp)
target_link_libraries(03-urls onion onioncpp)


add_custom_command(
   OUTPUT _04_templates_html.c
   COMMAND ${OTEMPLATE} ${CMAKE_CURRENT_SOURCE_DIR}/04-templates.html
                 ${CMAKE_CURRENT_BINARY_DIR}/_04_templates_html.c
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/04-templates.html
   )

add_executable(04-templates 04-templates.cpp _04_templates_html.c)
target_link_libraries(04-templates onion onioncpp)

add_executable(05-exportlocal 05-exportlocal)
target_link_libraries(05-exportlocal onion onioncpp)

add_executable(06-mime 06-mime)
target_link_libraries(06-mime onion onioncpp)

