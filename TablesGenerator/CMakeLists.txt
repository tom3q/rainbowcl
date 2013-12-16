set(SRC
	main.c
)

add_executable(${GENERATOR_NAME} main.c ${SRC})
target_link_libraries(${GENERATOR_NAME} ${SHAREDLIB_NAME})
target_link_libraries(${GENERATOR_NAME} ${SFMT_NAME})

# OpenMP
find_package( OpenMP REQUIRED)
if(OPENMP_FOUND)
	message("OPENMP FOUND")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()