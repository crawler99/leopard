cmake_minimum_required(VERSION 2.8)

# GCC settings so we can use the lastest features
# NOTE: The compiler must be set before any project.
set(CMAKE_C_COMPILER "${GCC_ROOT}/bin/gcc" CACHE PATH "Path to C compiler")
set(CMAKE_CXX_COMPILER "${GCC_ROOT}/bin/g++" CACHE PATH "Path to C++ compiler")

# Start project
project(leopard)

# Compilation env settings
function(Add_LD_LIBRARY_PATH PATH)
    string(FIND ENV{LD_LIBRARY_PATH} "${PATH}" _PATH_IDX)
    if (_PATH_IDX EQUAL -1)
        set(ENV{LD_LIBRARY_PATH} "${PATH}:$ENV{LD_LIBRARY_PATH}" PARENT_SCOPE)
    endif()
endfunction()

Add_LD_LIBRARY_PATH("${GCC_ROOT}/lib")
Add_LD_LIBRARY_PATH("${GCC_ROOT}/lib64")

set(CMAKE_CXX_FLAGS "-g -Wall")

add_compile_options(-std=c++14)

# NOTE: these linker variables should be after 'project' command
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -pthread")
set(CMAKE_CXX_CREATE_SHARED_LIBRARY "${CMAKE_CXX_CREATE_SHARED_LIBRARY} -pthread")

# Enable testing (this should be before 'add_subdirectory' command)
if (${BUILD_TESTS})
  enable_testing()
endif()

# Add test dirs
add_subdirectory(utils)
