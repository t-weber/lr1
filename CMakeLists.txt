#
# @author Tobias Weber (orcid: 0000-0002-7230-1932)
# @date june-2020
# @license see 'LICENSE.EUPL' file
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
# parser creation library
add_library(lr1 STATIC
	src/symbol.cpp src/symbol.h
	src/lr1_element.cpp src/lr1_closure.cpp src/lr1_collection.cpp src/lr1.h
	src/ll1.cpp src/ll1.h
	src/helpers.h src/common.h
)

target_link_libraries(lr1 ${Boost_LIBRARIES})
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# parser runtime library
add_library(lr1-rt STATIC
	src/symbol.cpp src/symbol.h
	src/lexer.cpp src/lexer.h
	src/parser.cpp src/parser.h
	src/ast.h
	src/ast_printer.cpp src/ast_printer.h
	src/helpers.h src/common.h
)

target_link_libraries(lr1-rt ${Boost_LIBRARIES})
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# tests
add_executable(test1 tests/tst1.cpp)
target_link_libraries(test1 lr1)

add_executable(expr_test_simplified tests/expr_simplified.cpp)
target_link_libraries(expr_test_simplified lr1 lr1-rt)

add_executable(expr_test_simplified_prec tests/expr_simplified_prec.cpp)
target_link_libraries(expr_test_simplified_prec lr1 lr1-rt)

add_executable(expr_test_simplified_ll1 tests/expr_simplified_ll1.cpp)
target_link_libraries(expr_test_simplified_ll1 lr1)

add_executable(expr_test_ll1 tests/expr_ll1.cpp)
target_link_libraries(expr_test_ll1 lr1)

add_executable(expr_test_create tests/expr.cpp)
target_compile_definitions(expr_test_create PUBLIC -DCREATE_PARSER)
target_link_libraries(expr_test_create lr1)

add_executable(expr_test_run tests/expr.cpp)
target_compile_definitions(expr_test_run PUBLIC -DRUN_PARSER)
target_link_libraries(expr_test_run lr1-rt)

add_executable(expr_test_prec_create tests/expr_prec.cpp)
target_compile_definitions(expr_test_prec_create PUBLIC -DCREATE_PARSER)
target_link_libraries(expr_test_prec_create lr1)

add_executable(expr_test_prec_run tests/expr_prec.cpp)
target_compile_definitions(expr_test_prec_run PUBLIC -DRUN_PARSER)
target_link_libraries(expr_test_prec_run lr1-rt)
# -----------------------------------------------------------------------------
