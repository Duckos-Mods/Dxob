cmake_minimum_required(VERSION 3.28.0)
project(DxobLib VERSION 0.1.0 LANGUAGES CXX) # Change target name to DxobLib
set(CMAKE_CXX_STANDARD 23)
set(DXOB_PROJNAME "DxobLib")

file(GLOB_RECURSE DXOB_SOURCES "*.cpp")
file(GLOB_RECURSE DXOB_HEADERS "*.hpp")

if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-W4 -Wextra -Wpedantic)
endif()
if (MSVC)
    add_compile_options(/WX)
else()
    add_compile_options(-Werror)
endif()
if (MSVC)
    add_compile_options(/sdl)
else()
    add_compile_options(-fstack-protector-strong)
endif()
if (MSVC)
    add_compile_options(/MP)
else()
    add_compile_options(-j)
endif()

add_library(${DXOB_PROJNAME} ${DXOB_SOURCES} ${DXOB_HEADERS})

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("/arch:AVX2" COMPILER_SUPPORTS_AVX2)
if(COMPILER_SUPPORTS_AVX2)
    add_compile_options("/arch:AVX2")
endif()

if (MSVC)
    source_group(
		TREE "${CMAKE_CURRENT_SOURCE_DIR}"
	)
endif()