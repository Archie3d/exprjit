##
## Scan the source folder recursively to identify all
## the source files *.h, *.c, *.hpp, *.cpp
##
macro(PARSE_SRC)
    file(GLOB_RECURSE SRC_CPP ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
    file(GLOB_RECURSE SRC_C ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
    file(GLOB_RECURSE SRC_H ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
    file(GLOB_RECURSE SRC_HPP ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp)
    file(GLOB_RECURSE SRC_H_PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
    file(GLOB_RECURSE SRC_HPP_PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp)
    file(GLOB_RECURSE RCS ${CMAKE_CURRENT_SOURCE_DIR}/resource/*.rc)

	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
	
	# Process Windows resources
	set(WINRC "")
	if(WIN32)
		set(WINRC ${RCS})
	endif()

    # Process dependencies
	PARSE_DEPENDENCIES()
	
    # Copy precompiled DLLs and LIBs if any
    file(COPY ${LIBS} DESTINATION ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
    file(COPY ${DLLS} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    install(FILES ${DLLS} DESTINATION ${INSTALL_BIN_PATH} COMPONENT runtime)
endmacro(PARSE_SRC)

##
## Link dependencies for the current target.
## This will also check that dependency has been compiled
## before this target.
##
macro(PARSE_DEPENDENCIES)
    foreach(DEP ${DEPENDS})
    if(${DEP}_INCLUDES)
        include_directories(${${DEP}_INCLUDES})
        link_libraries(${${DEP}_LIBS})
    else()
        message(FATAL_ERROR "Dependency package ${DEP} required by target ${PROJECT_NAME} has not been processed yet.")
    endif()
    endforeach()
endmacro(PARSE_DEPENDENCIES)

##
## Parse the unit tests.
##
macro(PARSE_TESTS)

    if(UTEST_FRAMEWORK_INCLUDES)
        # Catch package has been processed already
    else()
        message(FATAL_ERROR "Unit tests package 'catch' has not been processed yet.")
    endif()

    set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)
    if(CMAKE_BUILD_TESTS AND EXISTS ${TEST_DIR})
        # Scan all the unit tests
        file(GLOB TEST_SRCS ${TEST_DIR}/*.cpp)
        foreach(TEST_CPP ${TEST_SRCS})
            get_filename_component(FILENAME ${TEST_CPP} NAME_WE)

            # Unit test target
            set(TEST_TARGET test_${PROJECT_NAME}_${FILENAME})
            include_directories(${UTEST_FRAMEWORK_INCLUDES})
            include_directories(${TEST_DIR})
            include_directories(${${PROJECT_NAME}_INCLUDES})
            add_executable(${TEST_TARGET} ${TEST_CPP})

            # Make sure the 'Catch' framework generates main() entry point.
            add_definitions(-DCATCH_CONFIG_MAIN)
            
            # Link with the component and its dependencies
            target_link_libraries(${TEST_TARGET} ${PROJECT_NAME} ${${PROJECT_NAME}_LIBS})

            foreach(DEP ${DEPENDS})
                if(NOT ${DEP}_HEADERS_ONLY)
                    target_link_libraries(${TEST_TARGET} ${DEP})
                    target_link_libraries(${TEST_TARGET} ${${DEP}_LIBS})
                endif()
            endforeach()

            # Register this unit test with CTest
            add_test(${TEST_TARGET} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TEST_TARGET})
        endforeach()
    endif()

    # Copy test resources
    set(TEST_RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test/test_resources)
    if (CMAKE_BUILD_TESTS AND EXISTS ${TEST_RESOURCES_DIR})
        file(COPY ${TEST_RESOURCES_DIR} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    endif()
endmacro(PARSE_TESTS)

##
## Build an executable.
##
macro(BUILD_EXECUTABLE)    
    parse_src()

    add_executable(${PROJECT_NAME}
        ${SRC_CPP} ${SRC_C}
        ${SRC_HPP} ${SRC_H}
        ${SRC_HPP_PRIVATE} ${SRC_H_PRIVATE}
        ${WINRC}
    )

    link_libs()

	install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${INSTALL_BIN_PATH} COMPONENT runtime)

endmacro(BUILD_EXECUTABLE)

##
## Build a static library
##
macro(BUILD_STATIC_LIBRARY)

    parse_src()

	# Cache the library public includes folder
	set(${PROJECT_NAME}_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/include" CACHE INTERNAL "${PROJECT_NAME} library includes" FORCE)
	
    add_library(${PROJECT_NAME}
        ${SRC_CPP} ${SRC_C}
        ${SRC_HPP} ${SRC_H}
        ${SRC_HPP_PRIVATE} ${SRC_H_PRIVATE}
    )

    parse_tests()

    install(TARGETS ${PROJECT_NAME}
            RUNTIME DESTINATION ${INSTALL_BIN_PATH}
            ARCHIVE DESTINATION ${INSTALL_LIB_PATH}
            LIBRARY DESTINATION ${INSTALL_LIB_PATH}
            COMPONENT development)

    install_includes()

endmacro(BUILD_STATIC_LIBRARY)

##
## Build a shared library (DLL)
##
macro(BUILD_SHARED_LIBRARY)

    parse_src()

    	# Cache the library public includes folder
	set(${PROJECT_NAME}_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/include" CACHE INTERNAL "${PROJECT_NAME} library includes" FORCE)
	
    add_library(${PROJECT_NAME} SHARED
        ${SRC_CPP} ${SRC_C}
        ${SRC_HPP} ${SRC_H}
        ${SRC_HPP_PRIVATE} ${SRC_H_PRIVATE}
        ${WINRC}
    )

    parse_tests()

    install(TARGETS ${PROJECT_NAME}
            RUNTIME DESTINATION ${INSTALL_BIN_PATH}
            ARCHIVE DESTINATION ${INSTALL_LIB_PATH}
            LIBRARY DESTINATION ${INSTALL_LIB_PATH}
            COMPONENT development)

    install_includes()

endmacro(BUILD_SHARED_LIBRARY)

##
## Library that does not require any compilation.
##
macro(BUILD_HEADERS_ONLY_LIBRARY)
    # Cache the library public includes folder
    set(${PROJECT_NAME}_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/include" CACHE INTERNAL "${PROJECT_NAME} library includes" FORCE)
    set(${PROJECT_NAME}_HEADERS_ONLY TRUE CACHE INTERNAL "No target generated for ${PROJECT_NAME}" FORCE)
endmacro()

##
## Link dependent libraries
##
macro(LINK_LIBS)

    # Link the dependencies
    target_link_libraries(${PROJECT_NAME} ${${PROJECT_NAME}_LIBS})

    # Link with precompiled libraries
    foreach(DEP ${DEPENDS})
        if(NOT ${DEP}_HEADERS_ONLY)
            target_link_libraries(${PROJECT_NAME} ${DEP})
            target_link_libraries(${PROJECT_NAME} ${${DEP}_LIBS})
        endif()
    endforeach()

endmacro(LINK_LIBS)

##
## Install include files
##
macro(INSTALL_INCLUDES)

    set(PROJECT_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}")
    if(EXISTS ${PROJECT_INCLUDES})
        install(DIRECTORY ${PROJECT_INCLUDES}
            DESTINATION ${INSTALL_INCLUDE_PATH}
            COMPONENT development
        )
    endif()

    file(GLOB OTHER_H ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

    install(FILES ${OTHER_H}
        DESTINATION ${INSTALL_INCLUDE_PATH}/${PROJECT_NAME}
        COMPONENT development
    )

endmacro(INSTALL_INCLUDES)
