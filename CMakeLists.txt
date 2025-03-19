cmake_minimum_required (VERSION 3.28)

#may be empty for some unknown reasons, just compile with clang pls
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
else()
	message(warning ": compiler is ${CMAKE_CXX_COMPILER_ID}")
endif()

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("rulang"
	VERSION 0.0.0
	DESCRIPTION "Rulang programming language"
	LANGUAGES CXX
)

add_compile_options(--std=gnu++2c)

set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set (SOURCES "src/rulang.cpp" "src/rulang.hpp" "src/lexer.hpp" "src/lexer/lex.cpp"
		"src/lexer/lex_raw.cpp" "src/ast/ast.hpp" "src/generator.hpp" "src/occurance.hpp"
		"src/util.hpp" "src/interfaces.hpp"
		src/parser.hpp src/parser.cpp
        src/lexer/rule_number.cpp
		src/lexer/precedence.cpp
)
set (TESTS test_lexer_${PROJECT_NAME})


add_executable (${PROJECT_NAME} ${SOURCES} "src/main.cpp")
add_executable (test_lexer_${PROJECT_NAME} ${SOURCES}  "test/test_lexer/lexer.cpp" "test/main.cpp")

target_precompile_headers(${PROJECT_NAME} PRIVATE "src/rulang.hpp" "src/ast/ast.hpp")

#tests
foreach (Test ${TESTS})
	target_precompile_headers(${Test} REUSE_FROM ${PROJECT_NAME})
	add_test (${Test} ${Test})
endforeach()

enable_testing ()

#doxygen
find_package(Doxygen OPTIONAL_COMPONENTS mscgen dia dot)
if (DOXYGEN_FOUND)
	set(DOXYGEN_GENERATE_HTML YES)
	set(DOXYGEN_OUT ./docs)
	doxygen_add_docs(doc ${CMAKE_SOURCE_DIR})
	message(STATUS "Found doxygen ${DOXYGEN_VERSION}")
else()
	message(STATUS "Didn't find doxygen")
endif(DOXYGEN_FOUND)

# llvm
find_package(LLVM 20 CONFIG REQUIRED)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "LLVM definitions: ${LLVM_DEFINITIONS}")
llvm_map_components_to_libnames(LLVM_LIBS support core)
message(STATUS "LLVM libraries: ${LLVM_LIBS}")

target_include_directories(${PROJECT_NAME} PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LLVM_LIBS})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${LLVM_DEFINITIONS})

foreach (Test ${TESTS})
	target_include_directories(${Test} PRIVATE ${LLVM_INCLUDE_DIRS})
	target_link_libraries(${Test} PRIVATE ${LLVM_LIBS})
	target_compile_definitions(${Test} PRIVATE ${LLVM_DEFINITIONS})
endforeach()

#boost

set(Boost_DEBUG OFF)
find_package(Boost 1.86.0 REQUIRED COMPONENTS ALL)

message(STATUS "Boost found: ${Boost_VERSION}")
message(STATUS "Boost libraries directory: ${Boost_LIBRARY_DIRS}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
message(STATUS "Boost diag defs: ${Boost_LIB_DIAGNOSTIC_DEFINITIONS}")
message(STATUS "Boost include directory: ${Boost_INCLUDE_DIRS}")

target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${Boost_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${Boost_LIB_DIAGNOSTIC_DEFINITIONS})

foreach (Test ${TESTS})
	target_include_directories(${Test} PRIVATE ${Boost_INCLUDE_DIRS})
	target_link_directories(${Test} PRIVATE ${Boost_LIBRARY_DIRS})
	target_link_libraries(${Test} PRIVATE ${Boost_LIBRARIES})
	target_compile_definitions(${Test} PRIVATE ${Boost_LIB_DIAGNOSTIC_DEFINITIONS})
endforeach()
