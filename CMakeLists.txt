cmake_minimum_required(VERSION 3.5)

project(exprjit LANGUAGES CXX)

# Path to look for custom CMake scripts
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Enable unit tests
set(CMAKE_BUILD_TESTS ON CACHE BOOL "Build unit tests")

# Target folders
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/target)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/target/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

# Look for libraries in the target output folder
link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

# Installation paths
set(INSTALL_BIN_PATH ${CMAKE_INSTALL_PREFIX}/bin)
set(INSTALL_LIB_PATH ${CMAKE_INSTALL_PREFIX}/lib)
set(INSTALL_INCLUDE_PATH ${CMAKE_INSTALL_PREFIX}/include)

# gcc/MSVC output suffix
set(DYLIB_SUFFIX "so")
set(LIB_SUFFIX "a")

if(MINGW)
    set(DYLIB_SUFFIX "dll")
elseif(MSVC)
    set(DYLIB_SUFFIX "dll")
    set(LIB_SUFFIX "lib")
endif()

# Guess the target platform bits
if (MSVC)
	if (CMAKE_CL_64)
		set(TARGET_PLATFORM_64 TRUE)
	else()
		set(TARGET_PLATFORM_64 FALSE)
	endif()
endif()

if (NOT TARGET_PLATFORM_64)
    if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        set(TARGET_PLATFORM_64 TRUE)
    else()
        set(TARGET_PLATFORM_64 FALSE)
    endif()
endif()

if (${TARGET_PLATFORM_64})
    add_definitions(-D__x86_64__)
endif()

# Building for x86_64 shared modules required -fPIC flag
if(UNIX AND NOT WIN32)
    find_program(CMAKE_UNAME uname /bin /usr/bin /usr/local/bin)
    if(CMAKE_UNAME)
        exec_program(uname ARGS -m OUTPUT_VARIABLE CMAKE_SYSTEM_PROCESSOR)
        set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR} CACHE INTERNAL "processor type (i386 and x86_64)")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
            add_definitions(-fPIC)
        endif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    endif(CMAKE_UNAME)
endif(UNIX AND NOT WIN32)

# Enforce static linking
if(MSVC)
    add_compile_options(/MP)

    set(COMPILER_FLAGS
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO
    )

    foreach(flag ${COMPILER_FLAGS})
        string(REPLACE "/MD" "/MT" "${flag}" "${${flag}}")
        string(REPLACE "/Zi" "/ZI" "${flag}" "${${flag}}")
    endforeach()
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Enable unit-testing
include(CTest)

# Enable custom macros
include(macros)

# Activate C++ features
if(MSVC)
    # Enable exceptions for MSVC compiler
    set(CMAKE_CXX_FLAGS "/EHsc /bigobj /std:c++17" CACHE STRING "C++ Compile flags" FORCE)
else()
    set(CMAKE_CXX_FLAGS "-std=c++17" CACHE STRING "C++ Compile flags" FORCE)
endif()

# Process the project tree
add_subdirectory(externals)
add_subdirectory(source)
