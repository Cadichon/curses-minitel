cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(curses-minitel)

find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(cJSON CONFIG REQUIRED)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_BUILD_TYPE "Debug")

add_executable(curses-minitel src/main.c)

target_link_libraries(curses-minitel PRIVATE form)
target_link_libraries(curses-minitel PRIVATE ncurses)
target_link_libraries(curses-minitel PRIVATE unofficial::sqlite3::sqlite3)
target_link_libraries(curses-minitel PRIVATE cjson)