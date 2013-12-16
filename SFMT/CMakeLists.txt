set(SRC
        SFMT.c
)

file(GLOB INC "*.h")
add_library(${SFMT_NAME} ${SRC} ${INC}) 
