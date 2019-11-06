cmake_minimum_required(VERSION 3.5)
project(ecl3-lib LANGUAGES C CXX)

set(ECL3_LIB_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "")

set(CMAKE_CXX_STANDARD 11)

add_library(ecl3
    src/keyword.cpp
    src/summary.cpp
)
add_library(ecl3::ecl3 ALIAS ecl3)
target_include_directories(ecl3
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        # Work-around as described in [1]. Even privately linked interface
        # libraries must be exported, because cmake does not check if anything
        # other than include-paths are set. The build-local export file is
        # given to the python build script, so endianness must also be
        # exported.
        #
        # in time, this hack should go away, either by promotoing endianness to
        # a library in its own right, or hard-code the include path
        # (effectively what is done now)
        # [1] https://gitlab.kitware.com/cmake/cmake/issues/18049
        $<TARGET_PROPERTY:endianness::endianness,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_options(ecl3
    BEFORE
    PRIVATE
        $<$<CONFIG:Debug>:${PEDANTIC_COMPILE_FLAGS}>
        $<$<CONFIG:Debug>:${WERROR_FLAG}>
        $<$<CONFIG:RelWithDebInfo>:${PEDANTIC_COMPILE_FLAGS}>
)

install(
    TARGETS ecl3
    EXPORT ecl3-export
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION include)
install(
    EXPORT
        ecl3-export
    NAMESPACE
        ecl3::
    DESTINATION
        ${CMAKE_INSTALL_DATAROOTDIR}/ecl3/cmake
    FILE
        ecl3-config.cmake
)
export(
    TARGETS
        ecl3
    NAMESPACE
        ecl3::
    FILE
        ecl3-config.cmake
)

if (NOT BUILD_TESTING)
    return()
endif ()

add_executable(ecl3-tests
    tests/tests.cpp
    tests/keyword.cpp
    tests/summary.cpp
)
target_link_libraries(ecl3-tests ecl3 endianness::endianness ecl3::catch2)
add_test(NAME ecl3-tests COMMAND ecl3-tests)
