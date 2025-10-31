cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

# --- Prefer Homebrew LLVM toolchain ---
# Adjust version if needed, e.g., llvm@18, llvm@19, llvm@21, etc.
set(LLVM_ROOT "/opt/homebrew/opt/llvm" CACHE PATH "Path to Homebrew LLVM")

if(EXISTS "${LLVM_ROOT}/bin/clang++")
    message(STATUS "Using Homebrew LLVM toolchain at ${LLVM_ROOT}")
    set(CMAKE_C_COMPILER "${LLVM_ROOT}/bin/clang" CACHE FILEPATH "C compiler" FORCE)
    set(CMAKE_CXX_COMPILER "${LLVM_ROOT}/bin/clang++" CACHE FILEPATH "C++ compiler" FORCE)
else()
    message(WARNING "Homebrew LLVM not found at ${LLVM_ROOT}, using system compiler.")
endif()

# ---- macOS specific settings for std::ranges support ----
# if(APPLE)
#   # Use brewed Clang if available for better C++20 support
#   find_program(BREW_CLANGXX clang++ PATHS /usr/local/bin /opt/homebrew/opt/llvm/bin)
#   if(BREW_CLANGXX)
#     set(CMAKE_CXX_COMPILER ${BREW_CLANGXX})
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -I/opt/homebrew/include/c++/v1")
#     # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/opt/homebrew/Cellar/llvm/21.1.3/lib/c++")
#     set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -L/opt/homebrew/Cellar/llvm/21.1.3/lib/c++")
#     set(CMAKE_CXX_STANDARD_REQUIRED ON)
#     set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#   endif()
# endif()

project(
    bloomers
    VERSION 0.41.1
    DESCRIPTION "Basic bloom filter implementation"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- macOS / LLVM libc++ setup ----
if(APPLE)
    # Force use of libc++ from Homebrew LLVM
    add_compile_options(-stdlib=libc++ -I${LLVM_ROOT}/include/c++/v1)
    add_link_options(-stdlib=libc++ -L${LLVM_ROOT}/lib/c++)
endif()

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    bloomers_lib STATIC
    src/bloom.cpp
)

target_include_directories(
    bloomers_lib ${warning_guard}
    PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)

target_compile_features(bloomers_lib PUBLIC cxx_std_20)

# ---- Declare executable ----

add_executable(bloomers_exe src/main.cpp)
add_executable(bloomers::exe ALIAS bloomers_exe)

set_property(TARGET bloomers_exe PROPERTY OUTPUT_NAME bloomers)

target_compile_features(bloomers_exe PRIVATE cxx_std_20)

target_link_libraries(bloomers_exe PRIVATE bloomers_lib)

# --- FetchContent ---
include(FetchContent)

FetchContent_Declare(
  cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
  GIT_TAG        v3.3.1
)

FetchContent_MakeAvailable(cxxopts)

target_link_libraries(bloomers_exe PRIVATE cxxopts)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Enable testing ----
include(CTest)
enable_testing()

# Add test directory
add_subdirectory(test)

# ---- Developer mode ----

if(NOT bloomers_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of bloomers"
  )
endif()

include(cmake/dev-mode.cmake)
