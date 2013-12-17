set(SRC
        main.c
)

add_executable(${REDUCEGEN_NAME} main.c ${SRC})
target_link_libraries(${REDUCEGEN_NAME} ${SHAREDLIB_NAME})
target_link_libraries(${REDUCEGEN_NAME} ${SFMT_NAME})
