#
# @author Tobias Weber (orcid: 0000-0002-7230-1932)
# @date june-2020
# @license see 'LICENSE.EUPL' file
#

cmake_minimum_required(VERSION 3.0)

project(lr1)
enable_language(CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE ON)

add_definitions(-Wall -Wextra -Weffc++)


option(BUILD_EXAMPLES "build example programs" TRUE)
option(BUILD_TESTS "build test programs" TRUE)


find_package(Boost REQUIRED)
add_definitions(${Boost_CXX_FLAGS})

find_package(Threads REQUIRED)


include_directories(
	"${PROJECT_SOURCE_DIR}"
	"${PROJECT_SOURCE_DIR}/src"
	"${CMAKE_CURRENT_BINARY_DIR}"
	"${Boost_INCLUDE_DIRS}" "${Boost_INCLUDE_DIRS}/.."
)


# -----------------------------------------------------------------------------
# parser creation library
add_library(lr1-parsergen STATIC
	src/parsergen/symbol.cpp src/parsergen/symbol.h
	src/parsergen/lr1_element.cpp src/parsergen/lr1_closure.cpp
	src/parsergen/lr1_collection.cpp src/parsergen/lr1.h
	src/parsergen/ll1.cpp src/parsergen/ll1.h
	src/parsergen/helpers.h src/parsergen/common.h
)

target_link_libraries(lr1-parsergen ${Boost_LIBRARIES})
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# code generation library
add_library(lr1-codegen STATIC
	src/parsergen/symbol.cpp src/parsergen/symbol.h
	src/parsergen/helpers.h src/parsergen/common.h
	src/codegen/lexer.cpp src/codegen/lexer.h
	src/codegen/parser.cpp src/codegen/parser.h
	src/codegen/ast.h src/vm/opcodes.h src/vm/types.h
	src/codegen/ast_printer.cpp src/codegen/ast_printer.h
	src/codegen/ast_asm.cpp src/codegen/ast_asm.h
	src/codegen/sym.h
)

target_link_libraries(lr1-codegen ${Boost_LIBRARIES})
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# vm library
add_library(lr1-vm STATIC
	src/vm/vm.cpp src/vm/vm.h
	src/vm/vm_extfuncs.cpp
	src/vm/opcodes.h src/vm/helpers.h
)

target_link_libraries(lr1-vm ${Boost_LIBRARIES} Threads::Threads)
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# examples
if(BUILD_EXAMPLES)
	add_executable(vm src/examples/vm.cpp)
	target_link_libraries(vm lr1-vm)


	add_executable(script_create src/examples/script.cpp)
	target_compile_definitions(script_create PUBLIC -DCREATE_PARSER)
	target_link_libraries(script_create lr1-parsergen)

	add_executable(script_run src/examples/script.cpp)
	target_compile_definitions(script_run PUBLIC -DRUN_PARSER)
	target_link_libraries(script_run lr1-codegen lr1-vm)


	add_executable(expr_prec_create src/examples/expr_prec.cpp)
	target_compile_definitions(expr_prec_create PUBLIC -DCREATE_PARSER)
	target_link_libraries(expr_prec_create lr1-parsergen)

	add_executable(expr_prec_run src/examples/expr_prec.cpp)
	target_compile_definitions(expr_prec_run PUBLIC -DRUN_PARSER)
	target_link_libraries(expr_prec_run lr1-codegen lr1-vm)
endif()
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# tests
if(BUILD_TESTS)
	add_executable(test1 tests/tst1.cpp)
	target_link_libraries(test1 lr1-parsergen)

	add_executable(left_recursion tests/left_recursion.cpp)
	target_link_libraries(left_recursion lr1-parsergen)

	add_executable(right_recursion tests/right_recursion.cpp)
	target_link_libraries(right_recursion lr1-parsergen)


	add_executable(expr_simplified tests/expr_simplified.cpp)
	target_link_libraries(expr_simplified lr1-parsergen lr1-codegen)

	add_executable(expr_simplified_prec tests/expr_simplified_prec.cpp)
	target_link_libraries(expr_simplified_prec lr1-parsergen lr1-codegen)

	add_executable(expr_simplified_ll1 tests/expr_simplified_ll1.cpp)
	target_link_libraries(expr_simplified_ll1 lr1-parsergen)

	add_executable(expr_ll1 tests/expr_ll1.cpp)
	target_link_libraries(expr_ll1 lr1-parsergen)


	add_executable(expr_create tests/expr.cpp)
	target_compile_definitions(expr_create PUBLIC -DCREATE_PARSER)
	target_link_libraries(expr_create lr1-parsergen)

	add_executable(expr_run tests/expr.cpp)
	target_compile_definitions(expr_run PUBLIC -DRUN_PARSER)
	target_link_libraries(expr_run lr1-codegen)
endif()
# -----------------------------------------------------------------------------
