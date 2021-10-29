cmake_minimum_required(VERSION 3.18)

project(json VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-extra-arg=-std=c++17")

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE include)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/79efd968bf7edb60667314750e101bbf99a0494e.zip
)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
	${PROJECT_NAME}_test
	test/test.cpp
)

if(MSVC)
	target_compile_options(${PROJECT_NAME}_test PRIVATE /W4 /WX)
else()
	target_compile_options(${PROJECT_NAME}_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_include_directories(${PROJECT_NAME}_test PRIVATE ${PROJECT_SOURCE_DIR}/include)

set_target_properties(
	${PROJECT_NAME}_test
	PROPERTIES CXX_CLANG_TIDY
	"clang-tidy;-extra-arg=-std=c++17;-checks=-cert-err58-cpp,-cppcoreguidelines-avoid-non-const-global-variables,-cppcoreguidelines-owning-memory,-fuchsia-statically-constructed-objects,-llvmlibc-restrict-system-libc-headers,-llvmlibc-callee-namespace,-llvmlibc-implementation-in-namespace"
)

target_link_libraries(
	${PROJECT_NAME}_test
	gtest_main
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_test)
