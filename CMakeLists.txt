cmake_minimum_required(VERSION 3.4)

project(ThreadPool LANGUAGES CXX)

add_library(thread_pool STATIC
	include/ThreadPool.hpp
	src/ThreadPool.cpp
)

target_include_directories(thread_pool PUBLIC include PRIVATE src)

