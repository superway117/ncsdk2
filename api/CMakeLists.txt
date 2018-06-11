# ----------------------------------------------------------------------------
#  Root CMake file for MVNC
# ----------------------------------------------------------------------------

# Disable in-source builds to prevent source tree corruption.
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "
FATAL: In-source builds are not allowed.
       You should create a separate directory for build files.
")
endif()

cmake_minimum_required(VERSION 2.6)

project(mvnc)

if (NOT MSVC)
  message(FATAL_ERROR "Only Support Windows now")
endif()


# ----------------------------------------------------------------------------
#  Autodetect if we are in a GIT repository
# ----------------------------------------------------------------------------
set (GIT_REPO FALSE)
find_package(Git QUIET)

if(GIT_FOUND)
  set (git_cmd "${GIT_EXECUTABLE}" rev-parse)
  set (git_arg "--is-inside-work-tree")
  message(STATUS "git cmd: ${git_cmd}")
  execute_process(COMMAND ${git_cmd} ${git_arg}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE git_result
    OUTPUT_VARIABLE git_output)
  if(git_output)
    set (GIT_REPO TRUE)
  endif()
endif()

message(STATUS "We are in a GIT repository: ${GIT_REPO}")

# ----------------------------------------------------------------------------
# Set variables
# ----------------------------------------------------------------------------
set(BUILD_MVNC_LIB true CACHE BOOL "Build MVNC library")
if (GIT_REPO)
  set(BUILD_EXAMPLES true CACHE BOOL "Build FULL_CHECK example")
endif()
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/output")

if (GIT_REPO)
  #set(MV_COMMON_BASE "${CMAKE_SOURCE_DIR}/../../../../common" CACHE PATH "common base directory")
  set(MV_COMMON_BASE "${CMAKE_SOURCE_DIR}/../../../../common")
  message(STATUS "1: ${MV_COMMON_BASE}")
else()
	#set(MV_COMMON_BASE "${CMAKE_SOURCE_DIR}/src/common" CACHE PATH "common base directory")
	set(MV_COMMON_BASE "${CMAKE_SOURCE_DIR}/src/common")
	message(STATUS "2: ${MV_COMMON_BASE}")
endif()

if(NOT EXISTS "${MV_COMMON_BASE}/components")
  message(FATAL_ERROR "Please set correct common base directory")
endif()

set(CMAKE_SUPPRESS_REGENERATION true)
if (GIT_REPO)
  file (STRINGS "${CMAKE_SOURCE_DIR}/../../ncsdk/setup-scripts/version.txt" BUILD_NUMBER)
else()
  file (STRINGS "${CMAKE_SOURCE_DIR}/src/version.txt" BUILD_NUMBER)
endif()
message(STATUS "BUILD_NUMBER:${BUILD_NUMBER}")

add_definitions(-D__PC__)
add_definitions(-DUSE_USB_VSC)
add_definitions(-DHAVE_STRUCT_TIMESPEC)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-DVERSION_NAME="${BUILD_NUMBER}")
add_definitions(-DDEVICE_SHELL_ENABLED)

if (BUILD_MVNC_LIB)
  add_subdirectory(src)
endif()
if (BUILD_EXAMPLES)
  if (GIT_REPO)
    add_subdirectory(examples/cdemo)
#  else()
#    add_subdirectory(cmake_examples/cdemo)
  endif()
endif()