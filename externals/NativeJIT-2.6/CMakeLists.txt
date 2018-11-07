project(nativejit)

# Remove the warning that WIN32 is not defined in Cygwin any more.
set(CMAKE_LEGACY_CYGWIN_WIN32 0)

# Project-wide settings for NativeJIT
project(NativeJIT CXX C)

if (WIN32 OR CYGWIN)
    set(NATIVEJIT_PLATFORM_POSIX 0)
    set(NATIVEJIT_PLATFORM_WINDOWS 1)
else()
    set(NATIVEJIT_PLATFORM_POSIX 1)
    set(NATIVEJIT_PLATFORM_WINDOWS 0)
endif()

if(NATIVEJIT_PLATFORM_WINDOWS)
  # Note: -D works for both MSVC and GCC/Clang.
  add_definitions(-DNATIVEJIT_PLATFORM_WINDOWS)  
else()
  add_definitions(-DNATIVEJIT_PLATFORM_POSIX)
endif()

build_static_library()