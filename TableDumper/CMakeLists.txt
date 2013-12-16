set(SRC
        main.c
)

add_executable(${DUMPER_NAME} main.c ${SRC})
target_link_libraries(${DUMPER_NAME} ${SHAREDLIB_NAME})
