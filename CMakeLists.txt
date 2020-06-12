#
# @author Tobias Weber
# @date june-2020
# @license: see 'LICENSE.EUPL' file
#

project(lr1)
cmake_minimum_required(VERSION 3.0)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE ON)

add_definitions(-Wall -Wextra -Weffc++)


find_package(Boost REQUIRED)
add_definitions(${Boost_CXX_FLAGS})


include_directories(
	"${PROJECT_SOURCE_DIR}"
	"${PROJECT_SOURCE_DIR}/src"
	"${CMAKE_CURRENT_BINARY_DIR}"
	"${Boost_INCLUDE_DIRS}" "${Boost_INCLUDE_DIRS}/.."
)


# -----------------------------------------------------------------------------
# tests
add_executable(test1
	src/lr1.cpp src/lr1.h src/symbol.cpp src/symbol.h
	tests/test1.cpp
)

target_link_libraries(test1 ${Boost_LIBRARIES})
# -----------------------------------------------------------------------------
