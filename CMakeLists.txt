cmake_minimum_required(VERSION 3.3)

if(MSVC)
  set(CMAKE_CXX_COMPILER cl)
else()
  set(CMAKE_CXX_STANDARD 14 CACHE STRING "C++ standards to use (11|14)")
  set(CMAKE_CXX_COMPILER g++ CACHE STRING "Compiler to use")
endif()

# project variables
project(redispack)
set(BIN_DIRS unit-test)
set(USE_STATIC OFF CACHE BOOL "Uses only external static libraries for linking")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Builds all non-executable source directories as shared libraries")

# libraries finding
if(${USE_STATIC})
  if(MSVC)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .lib)
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
  endif()
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_library(CPP_REDIS_LIB_DEBUG NAMES cpp_redis REQUIRED)
find_library(CPP_REDIS_LIB_RELEASE NAMES cpp_redis REQUIRED)
find_library(TACOPIE_LIB_DEBUG NAMES tacopie REQUIRED)
find_library(TACOPIE_LIB_RELEASE NAMES tacopie REQUIRED)
find_library(GTEST_LIB_DEBUG NAMES gtest REQUIRED)
find_library(GTEST_LIB_RELEASE NAMES gtest REQUIRED)

set(CPP_REDIS_LIB debug ${CPP_REDIS_LIB_DEBUG} optimized ${CPP_REDIS_LIB_RELEASE})
set(TACOPIE_LIB debug ${TACOPIE_LIB_DEBUG} optimized ${TACOPIE_LIB_RELEASE})
set(GTEST_LIB debug ${GTEST_LIB_DEBUG} optimized ${GTEST_LIB_RELEASE})

# project to libraries mapping
set(PROJ_LIBS_unit-test CPP_REDIS_LIB TACOPIE_LIB GTEST_LIB)

# project to locally built libraries mapping
set(LOCAL_PROJ_LIBS_unit-test)

# general fixed project variables
set(SRC_ROOT_DIR src)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# general fixed compiler settings
if(MSVC)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Wall")
else()
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
endif()

# performs static/shared libraries global settings
if(${USE_STATIC})
  if(MSVC)
    string(REGEX REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    string(REGEX REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
    string(REGEX REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_MINSIZEREL ${CMAKE_CXX_FLAGS_MINSIZEREL})
    string(REGEX REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
  else()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
  endif()
endif()

# recursive build
file(GLOB SRC_DIRS ${CMAKE_SOURCE_DIR}/${SRC_ROOT_DIR}/*)

# performs action on directories with source files
foreach(SRC_DIR ${SRC_DIRS})
  file(GLOB CXX_SRC_FILES ${SRC_DIR}/*.cpp ${SRC_DIR}/*.cxx)

  # filters out directories with no source files
  if(CXX_SRC_FILES)
    get_filename_component(PROJ_NAME ${SRC_DIR} NAME)

    # check if to build as executable or library
    if(PROJ_NAME IN_LIST BIN_DIRS)
      add_executable(${PROJ_NAME} ${CXX_SRC_FILES})
    else()
      add_library(${PROJ_NAME} ${CXX_SRC_FILES})
    endif()

    # add in all the include and library dependencies
    ## need to evaluate all the project libraries into the actual library file paths
    set(SUBST_PROJ_LIBS)

    if(PROJ_LIBS_${PROJ_NAME})
      foreach(PROJ_LIB ${PROJ_LIBS_${PROJ_NAME}})
        list(APPEND SUBST_PROJ_LIBS ${${PROJ_LIB}})
      endforeach()
    endif()

    target_include_directories(${PROJ_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/${SRC_ROOT_DIR})
    target_link_libraries(${PROJ_NAME} ${LOCAL_PROJ_LIBS_${PROJ_NAME}} ${SUBST_PROJ_LIBS} Threads::Threads)
  endif()
endforeach()
