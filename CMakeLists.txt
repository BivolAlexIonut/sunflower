cmake_minimum_required(VERSION 3.20)
project(sunflower CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# raylib — descărcat și compilat automat la primul build
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(raylib)

add_executable(sunflower
    src/main.cpp
    src/player.cpp
    src/tilemap.cpp
    src/farm.cpp
    src/inventory.cpp
)
target_link_libraries(sunflower PRIVATE raylib)

# Copiază folderul cu sprite-uri lângă executabil, ca path-urile relative să meargă.
add_custom_command(TARGET sunflower POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/sprites
        $<TARGET_FILE_DIR:sunflower>/sprites
)

# pe Windows, fereastră de joc fără consolă în spate (doar pe Release)
if(WIN32)
    set_target_properties(sunflower PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
endif()
