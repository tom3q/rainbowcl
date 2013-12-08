set(SRC
	md5.c
)

file(GLOB INC "*.h")
add_library(${SHAREDLIB_NAME} ${SRC} ${INC})