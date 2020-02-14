cmake_minimum_required(VERSION 3.12)
project(retask
        VERSION 0.1
        DESCRIPTION "Basic worker manager for tasks"
        HOMEPAGE_URL "https://github.com/reddec/retask"
        LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -Wpedantic")

add_subdirectory(apps)
add_subdirectory(src)
