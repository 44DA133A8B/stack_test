cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CMAKE_INTERPROCEDURAL_OPTIMIZATION "Enable LTO/IPO/LTCG" ON)

project(stack_test LANGUAGES CXX)

add_library(stack SHARED "stack.cpp" "stack.hpp")
source_group("" FILES "stack.cpp" "stack.hpp")
target_compile_definitions(stack PRIVATE STACK_EXPORT)

add_executable(test "test.cpp" "stack.hpp")
source_group("" FILES "test.cpp" "stack.hpp")
target_link_libraries(test PRIVATE stack)

if (MSVC)
	add_definitions(/GS-)
	string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT test)
endif()
