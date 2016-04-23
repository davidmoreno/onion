if(MD2MAN)
  add_custom_command(
     OUTPUT opack.1.gz
     COMMAND ${MD2MAN} ${CMAKE_CURRENT_SOURCE_DIR}/opack.md
                  | gzip > ${CMAKE_CURRENT_BINARY_DIR}/opack.1.gz
     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/opack.md
     )

  add_custom_command(
    OUTPUT otemplate.1.gz
    COMMAND ${MD2MAN} ${CMAKE_CURRENT_SOURCE_DIR}/otemplate.md
                  | gzip > ${CMAKE_CURRENT_BINARY_DIR}/otemplate.1.gz
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/otemplate.md
    )
  add_custom_command(
    OUTPUT onion-crl.1.gz
    COMMAND ${MD2MAN} ${CMAKE_CURRENT_SOURCE_DIR}/onion-crl.md
                  | gzip > ${CMAKE_CURRENT_BINARY_DIR}/onion-crl.1.gz
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/onion-crl.md
    )

  add_custom_target(
    manpages ALL
    DEPENDS
      ${CMAKE_CURRENT_BINARY_DIR}/opack.1.gz
      ${CMAKE_CURRENT_BINARY_DIR}/otemplate.1.gz
      ${CMAKE_CURRENT_BINARY_DIR}/onion-crl.1.gz
  )
  install(
    FILES
      ${CMAKE_CURRENT_BINARY_DIR}/opack.1.gz
      ${CMAKE_CURRENT_BINARY_DIR}/otemplate.1.gz
      ${CMAKE_CURRENT_BINARY_DIR}/onion-crl.1.gz
    DESTINATION ${CMAKE_INSTALL_MANDIR}
  )
endif(MD2MAN)
