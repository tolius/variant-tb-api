cmake_minimum_required(VERSION 3.16.0)

project(test_${VARIANT_NAME}_tb_api LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The include directory, the variant (ANTI / ATOMIC) and the libraries come from the
# library target
add_executable(${PROJECT_NAME}
	test_${VARIANT_NAME}.cpp
	crc_check.h
)

target_link_libraries(${PROJECT_NAME}
	${VARIANT_NAME}_tb_api
)
