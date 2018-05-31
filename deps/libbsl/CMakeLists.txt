cmake_minimum_required(VERSION 2.6)

project(hddl-bsl)

if(NOT MSVC)
set(SECURE_CFLAGS "-fPIC")
set(SECURE_LDFLAGS "-ldl")
set(CMAKE_CXX_FLAGS_DEBUG "-g3")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SECURE_CFLAGS} -Wall")
set(CMAKE_CXX_LDFLAGS "${CMAKE_CXX_LDFLAGS} ${SECURE_LDFLAGS}")
endif()

if(DEFINED CMAKE_INSTALL_PREFIX)
   message(STATUS "INSTALL_PATH is '${CMAKE_INSTALL_PREFIX}'.")
else()
if(NOT MSVC)
   set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
endif()

include_directories("./include")

if(MSVC)
add_subdirectory(json-c)
endif()
add_subdirectory(src)

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_GENERATOR "ZIP")

include(CPack)
