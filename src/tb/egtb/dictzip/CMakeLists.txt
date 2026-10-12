cmake_minimum_required (VERSION 3.13.0)

project(dictzip LANGUAGES C)

if(ARCH_FLAGS)
	if (WIN32)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch:${ARCH_FLAGS}")
	else()
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=${ARCH_FLAGS}")
	endif()
endif()

file(GLOB SRC_APP
	*.c
	*.h
)
set(APP_SRC ${SRC_APP})
SOURCE_GROUP ("src" FILES ${APP_SRC})

# zlib-ng 2.3.3 (source without test, doc, tools, configure, Makefile.in), zlib API
set(ZLIB_COMPAT ON)
set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)
set(WITH_GTEST OFF)
add_subdirectory(zlib-ng EXCLUDE_FROM_ALL)

add_library(${PROJECT_NAME} OBJECT
	${APP_SRC}
)
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE 1)
target_link_libraries(${PROJECT_NAME} PUBLIC zlib-ng::zlibstatic)

if (WIN32)
	target_compile_definitions(${PROJECT_NAME}
		PRIVATE _CRT_SECURE_NO_WARNINGS
		PRIVATE HAVE_MMAP
	)
else()
	target_compile_definitions(${PROJECT_NAME}
		PRIVATE HAVE_UNISTD_H
		PRIVATE HAVE_MMAP
		PRIVATE NDEBUG
	)
	target_compile_options(${PROJECT_NAME}
		PRIVATE "-Ofast"
		PRIVATE "-flto"
	)
endif()
