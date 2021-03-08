add_library(utils INTERFACE)

target_include_directories(utils INTERFACE include)

if (${BUILD_TESTING})
  add_subdirectory(tests)
endif()
