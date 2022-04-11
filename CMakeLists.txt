cmake_minimum_required(VERSION 3.8)

project(ThreadPool LANGUAGES CXX C)
set (CMAKE_CXX_STANDARD 17)

add_library(thread_pool STATIC
	include/ThreadPool.hpp
	src/ThreadPool.cpp
)

target_include_directories(thread_pool PUBLIC include PRIVATE src)

############################

enable_testing()

math(EXPR TARGET_X64 "(${CMAKE_SIZEOF_VOID_P} * 8) / 64")			# Это короче, чем if...
#set(TARGET_ARCH $<IF:${TARGET_X64},x64,x86>)						# Почему не получается так?
if(TARGET_X64)
	set(TARGET_ARCH x64)
else()
	set(TARGET_ARCH x86)
endif(TARGET_X64)

list(APPEND CMAKE_PREFIX_PATH
	../extSDK/googletest/${TARGET_ARCH}
)

find_package(GTest REQUIRED CONFIG)
message(STATUS "Using GTest ${GTest_VERSION}")

add_executable(thread_pool_tests
	tests/ThreadPoolTests.cpp
	tests/DifferentFunctorTypesTests.cpp
)
target_link_libraries(thread_pool_tests PRIVATE thread_pool GTest::gmock_main)
target_compile_definitions(thread_pool_tests PRIVATE UNICODE)
