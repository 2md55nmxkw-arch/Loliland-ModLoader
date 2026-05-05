cmake_minimum_required(VERSION 3.16)

project(LoliLoader
    VERSION   1.0.0
    LANGUAGES CXX
    DESCRIPTION "Universal LoliLand mod injector"
)

option(LOLILOADER_STATIC "Statically link the C/C++ runtime (portable .exe)" ON)

set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS         OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

if(WIN32)
    add_compile_definitions(UNICODE _UNICODE)
endif()

add_executable(LoliLoader LoliLoader.cpp)

if(MSVC)
    target_compile_options(LoliLoader PRIVATE /W4 /permissive- /utf-8)
    set_target_properties(LoliLoader PROPERTIES
        LINK_FLAGS "/ENTRY:wmainCRTStartup /SUBSYSTEM:CONSOLE"
    )
elseif(MINGW OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(LoliLoader PRIVATE -Wall -Wextra -Wpedantic)
    target_link_options(LoliLoader PRIVATE -municode)
endif()

if(WIN32)
    target_link_libraries(LoliLoader PRIVATE shell32 advapi32)
endif()

if(LOLILOADER_STATIC)
    if(MSVC)
        set_property(TARGET LoliLoader PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    elseif(MINGW OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_options(LoliLoader PRIVATE
            -static -static-libgcc -static-libstdc++)
    endif()
endif()

add_custom_command(TARGET LoliLoader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:LoliLoader>"
            "${CMAKE_SOURCE_DIR}/LoliLoader.exe"
    COMMENT "Copying LoliLoader.exe next to LOAD_MODS.bat"
)

message(STATUS "LoliLoader configured.")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler  : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Static    : ${LOLILOADER_STATIC}")
