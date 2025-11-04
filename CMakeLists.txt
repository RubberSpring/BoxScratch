cmake_minimum_required(VERSION 3.22)
project(boxscratch)

include(FetchContent)
include(cmake/CPM.cmake)
include(FindPkgConfig)

find_package(SDL2 REQUIRED)
PKG_SEARCH_MODULE(SDL2IMAGE REQUIRED SDL2_image)
include_directories(boxscratch ${SDL2_INCLUDE_DIRS})
include_directories(boxscratch ${SDL2_IMAGE_INCLUDE_DIRS})

add_executable(boxscratch src/main.cpp)
target_link_libraries(boxscratch ${SDL2_LIBRARIES})
target_link_libraries(boxscratch "-lSDL2_image")
target_include_directories(boxscratch PUBLIC include)
target_include_directories(boxscratch PUBLIC extern)

FetchContent_Declare(
    lunasvg
    GIT_REPOSITORY https://github.com/sammycage/lunasvg.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(lunasvg)
target_link_libraries(boxscratch lunasvg::lunasvg)

#FetchContent_Declare(
#  cpptrace
#  GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
#  GIT_TAG        v1.0.4
#)
#FetchContent_MakeAvailable(cpptrace)
#target_link_libraries(boxscratch cpptrace::cpptrace)

find_package(libzip REQUIRED)
target_link_libraries(boxscratch libzip::zip)