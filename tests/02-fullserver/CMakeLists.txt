
ADD_CUSTOM_COMMAND(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/01-userver.sh
	COMMAND cp ${CMAKE_SOURCE_DIR}/tests/02-fullserver/01-userver.sh ${CMAKE_CURRENT_BINARY_DIR}/01-userver.sh
	DEPENDS ${CMAKE_SOURCE_DIR}/tests/02-fullserver/01-userver.sh
	)

ADD_CUSTOM_TARGET(01-userver.sh DEPENDS userver)
