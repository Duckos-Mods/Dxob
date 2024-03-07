cmake_minimum_required(VERSION 3.28.0)
project(DxobMain VERSION 0.1.0 LANGUAGES CXX) 
set(CMAKE_CXX_STANDARD 23)
# set(DXOB_BUILD_TESTS ON CACHE BOOL "Build the tests for the Dxob library")

add_subdirectory(src)

if (NOT DEFINED DXOB_BUILD_TESTS)
    set(DXOB_BUILD_TESTS ON)
endif()

if(NOT DXOB_BUILD_TESTS)
	return()
endif()

message(STATUS "Building tests for Dxob library")

# Glob the test files
file(GLOB_RECURSE DXOB_TEST_FILES "test/*.cpp")
file(GLOB_RECURSE DXOB_TEST_HEADERS "test/*.hpp")

# include the src directory
include_directories(src)

# Add the test executable
add_executable(DxobTest ${DXOB_TEST_FILES} ${DXOB_TEST_HEADERS})


if (MSVC)
    source_group(
		TREE "${CMAKE_CURRENT_SOURCE_DIR}/test/"
	)
	# Set startup project to DxobTest
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT DxobTest)
endif()

# Link the Dxob library
target_link_libraries(DxobTest DxobLib)