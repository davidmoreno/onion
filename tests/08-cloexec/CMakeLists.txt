add_executable(01-cloexec 01-cloexec.c)
target_link_libraries(01-cloexec onion)

add_executable(testfds testfds.c ../../src/onion/log.c)


