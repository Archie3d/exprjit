project(catch)

set(UTEST_FRAMEWORK_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/include" CACHE INTERNAL "Unit tests framework based on ${PROJECT_NAME} library includes" FORCE)

build_headers_only_library()
