# Generate intellisense for tests:
# cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
# (Run ':LspRestart' if this doesn't work right away)

cmake_minimum_required(VERSION 3.14)

project(bloomersTests LANGUAGES CXX)

include(FetchContent)

# ---- Download GoogleTest ----
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ---- Helper function ----
function(add_bloomers_test name)
  set(src_path "${CMAKE_CURRENT_SOURCE_DIR}/src/${name}.cpp")
  if(EXISTS "${src_path}")
    add_executable(${name} ${src_path})
    target_link_libraries(${name} PRIVATE bloomers_lib GTest::gtest_main)
    target_compile_features(${name} PRIVATE cxx_std_20)
    target_include_directories(${name} PRIVATE ${PROJECT_SOURCE_DIR}/src)
    include(GoogleTest)
    gtest_discover_tests(${name})
  else()
    message(WARNING "Test source not found: ${src_path}, skipping target ${name}")
  endif()
endfunction()

# ---- Define your tests ----
add_bloomers_test(bloom_test)
