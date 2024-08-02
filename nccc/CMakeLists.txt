if(NOT NCCC_ROOT)
    # FIXME: Migrate to CMake module later.
    message(FATAL_ERROR "Set NCCC_ROOT for nccc directory")
endif()

cmake_minimum_required(VERSION 3.20)
project(miniio-nccc C)
include(${NCCC_ROOT}/stubsupport/cmake/NcccStubMacros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/stub_miniio.cmake)

include_directories(${CMAKE_CURRENT_BINARY_DIR} 
    ../include
    ${NCCC_ROOT}/stubsupport)

if(BUILD_NCCC_MODULE)
    add_definitions(-DNCCC_DLL)
    set(libtype MODULE)
else()
    set(libtype STATIC)
endif()

if("${CMAKE_SYSTEM_NAME}" STREQUAL WindowsStore)
    set(backend stub)
elseif(EMSCRIPTEN)
    set(backend localnetstub)
else()
    set(backend uv)
    if(NOT UV_ROOT)
        message(FATAL_ERROR "UV_ROOT required")
    endif()
    include_directories(${UV_ROOT}/include)
endif()

add_library(nccc_miniio ${libtype}
    ../src/miniio_${backend}.c
    nccc_miniio.c
    )

if(${backend} STREQUAL uv)
    target_link_libraries(nccc_miniio
        PRIVATE
        uv_a)
endif()
