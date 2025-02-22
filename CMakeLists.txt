cmake_minimum_required(VERSION 3.10)
project(HFHub CXX)

# Find CURL package
find_package(CURL REQUIRED)

include_directories(include)
include_directories(src)

# Define the hfhub static library and link it to CURL
add_library(hfhub STATIC src/downloader.cpp)
target_link_libraries(hfhub CURL::libcurl)

# Export the hfhub library and CURL as a package for other projects to use
export(TARGETS hfhub FILE "${CMAKE_BINARY_DIR}/hfhub-targets.cmake")
export(PACKAGE hfhub)

install(TARGETS hfhub
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/
  DESTINATION include/
)

# Define and install the hfhub_demo executable
add_executable(hfhub_demo src/main.cpp)
target_link_libraries(hfhub_demo hfhub)
install(TARGETS hfhub_demo DESTINATION bin)
