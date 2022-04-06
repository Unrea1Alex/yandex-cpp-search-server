cmake_minimum_required(VERSION 3.5)

project(SearchServerQT LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SearchServerSRC
     "*.h"
	 "*.cpp"
)

add_executable(SearchServerQT ${SearchServerSRC})

target_link_libraries(${PROJECT_NAME} PRIVATE tbb)