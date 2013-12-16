set(SRC
	md5.c
	utils.c
)

file(GLOB INC "*.h")
add_library(${SHAREDLIB_NAME} ${SRC} ${INC})