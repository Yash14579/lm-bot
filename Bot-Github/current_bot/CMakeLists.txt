cmake_minimum_required(VERSION 3.10)

project(LordsMobileBot VERSION 1.0.2 LANGUAGES C)

# Deep Windows source paths can make MSBuild FileTracker fail with FTK1011.
# Disable MSBuild file tracking for Visual Studio generators; this keeps builds
# working even when the project is extracted under a long Downloads path.
if(MSVC)
    set(CMAKE_VS_GLOBALS "TrackFileAccess=false")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(client
    src/main.c
    src/connection.c
    src/log.c
    src/protocol.c
    src/des.c
    src/map_point.c
    src/command.c
    src/bank.c
    src/config.c
    src/gathering.c
    src/automation.c
    src/activity_log.c
    src/settings_loader.c
    src/cJSON.c
    src/gameassets_loader.c
)


target_include_directories(client PRIVATE include)

if(WIN32)
    target_link_libraries(client PRIVATE ws2_32)
endif()

if(NOT WIN32)
    target_link_libraries(client PRIVATE m)
endif()
