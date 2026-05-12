cmake_minimum_required(VERSION 3.26)
project(template CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # usually helps with IDE intellisense for external libaries. should be regogized after building
set(CMAKE_CXX_CPPCHECK "cppcheck;--enable=all;--inline-suppr;--suppress=missingIncludeSystem;--quiet")

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/* ${CMAKE_CURRENT_SOURCE_DIR}/include/*)

#add_library(${PROJECT_NAME} ${SOURCES})
add_executable(${PROJECT_NAME} ${SOURCES})

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE 
        -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion -Wformat=2
    )
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)
