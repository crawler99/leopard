cmake_minimum_required(VERSION 3.1)

add_library(utils SHARED)

target_include_directories(utils PUBLIC include)

if (${BUILD_TESTS})
  message("Building tests for " ${PROJECT_NAME} " ...")

  include_directories(${GTEST_ROOT}/include)
  include_directories(${GMOCK_ROOT}/include)
  link_directories(${GTEST_ROOT}/lib)
  link_directories(${GMOCK_ROOT}/lib)

  file(GLOB TEST_SRCS "test/*.cxx")

  set(EXECUTABLE "runUnitTests_${PROJECT_NAME}")

  add_executable(${EXECUTABLE} ${TEST_SRCS})

  # Standard linking to gtest stuff.
  target_link_libraries(${EXECUTABLE} gtest gtest_main)

  # Extra linking for the project.
  # target_link_libraries(${EXECUTABLE} ${PROJECT_NAME})

  # Add tests
  add_test(NAME testAll COMMAND ${EXECUTABLE})
endif()

