cmake_minimum_required(VERSION 3.10)
project(HFHub CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++17 -O0 -g")

find_package(CURL REQUIRED)

include_directories(include)
include_directories(src)

add_library(hfhub STATIC src/downloader.cpp)
target_link_libraries(hfhub CURL::libcurl)
install(TARGETS hfhub
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(DIRECTORY include/
  DESTINATION include/
)

add_executable(hfhub_demo src/main.cpp)
target_link_libraries(hfhub_demo hfhub)
install(TARGETS hfhub_demo DESTINATION bin)
