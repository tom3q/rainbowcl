set(SRC
	main.c
)

add_executable(${CRACKER_NAME} main.c ${SRC})
target_link_libraries(${CRACKER_NAME} ${SHAREDLIB_NAME})