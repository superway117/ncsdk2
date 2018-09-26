#set MACRO
set(XLINK_PC "${MV_COMMON_BASE}/components/XLink/pc")
set(XLINK_SHARED "${MV_COMMON_BASE}/components/XLink/shared")
set(USB_WIN "${MV_COMMON_BASE}/components/USB_WIN")
set(WIN_PTHREAD "${MV_COMMON_BASE}/components/WinPthread")
set(COMMON_SHARE "${MV_COMMON_BASE}/shared/include/")

if (GIT_REPO)
#HIGHCLASS SRC LIST
  set(HIGHCLASS_SRC_LIST mvnc_api_highclass.c)
endif()

#COMMON SRC LIST
aux_source_directory(${XLINK_PC}/ XLINK_PC_LIST)
aux_source_directory(${XLINK_SHARED}/ XLINK_SHARED_LIST)
aux_source_directory(${USB_WIN}/ USB_WIN_LIST)
aux_source_directory(${WIN_PTHREAD}/ WIN_PTHREAD_LIST)
message(STATUS "SRC_LIST:${SRC_LIST}")
message(STATUS "XLINK_PC_LIST:${XLINK_PC_LIST}")
message(STATUS "XLINK_SHARED_LIST:${XLINK_SHARED_LIST}")
message(STATUS "USB_WIN_LIST:${USB_WIN_LIST}")
message(STATUS "WIN_PTHREAD:${WIN_PTHREAD_LIST}")

#MVNC SRC LIST
set(MVNC_SRC_LIST mvnc_api.c fp16.c)


#INCLUDE PATH
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories("${XLINK_PC}")
include_directories("${XLINK_SHARED}")
include_directories("${USB_WIN}")
include_directories("${WIN_PTHREAD}")
include_directories("${COMMON_SHARE}")


function(SET_TARGET_OUTPUT_DIRS target dir_archive dir_library dir_runtime)
  if(${CMAKE_VERSION} VERSION_GREATER "3.0.1")
    set_target_properties(
      ${target}
      PROPERTIES
      ARCHIVE_OUTPUT_DIRECTORY                "${CMAKE_BINARY_DIR}/build/${dir_archive}"
      ARCHIVE_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/build/${dir_archive}"
      ARCHIVE_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/build/${dir_archive}"
      ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_BINARY_DIR}/build/${dir_archive}"
      ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/build/${dir_archive}"
      LIBRARY_OUTPUT_DIRECTORY                "${CMAKE_BINARY_DIR}/build/${dir_library}"
      LIBRARY_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/build/${dir_library}"
      LIBRARY_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/build/${dir_library}"
      LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_BINARY_DIR}/build/${dir_library}"
      LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/build/${dir_library}"
      RUNTIME_OUTPUT_DIRECTORY                "${CMAKE_BINARY_DIR}/build/${dir_runtime}"
      RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/build/${dir_runtime}"
      RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/build/${dir_runtime}"
      RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_BINARY_DIR}/build/${dir_runtime}"
      RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/build/${dir_runtime}"
    )
  endif()
endfunction()


if (GIT_REPO)
  #BUILD HIGHCLASS
  add_library(mvnc_highclass SHARED ${HIGHCLASS_SRC_LIST} ${XLINK_PC_LIST} ${XLINK_SHARED_LIST} ${XLINKCONSOLE_PC_LIST} ${USB_WIN_LIST} ${WIN_PTHREAD_LIST})
  set_target_output_dirs(mvnc_highclass   lib lib bin)
  #INSTALL
  install(DIRECTORY ${CMAKE_BINARY_DIR}/build/ DESTINATION .
    FILES_MATCHING PATTERN "*"
    PATTERN "*.ilk" EXCLUDE
    PATTERN "*.pdb" EXCLUDE
    PATTERN "*.exp" EXCLUDE)
endif()


#BUILD MVNC
if (NOT GIT_REPO)
#  link_directories("$ENV{MVNC_DIR}\lib")
  set(CMAKE_PREFIX_PATH $ENV{MVNC_DIR})
  find_library(MVNC_HIGHCLASS_LIB NAMES mvnc_highclass HINTS "${CMAKE_PREFIX_PATH}/lib") 
  add_library(mvnc SHARED ${MVNC_SRC_LIST} ${XLINK_PC_LIST} ${XLINK_SHARED_LIST} ${XLINKCONSOLE_PC_LIST} ${USB_WIN_LIST} ${WIN_PTHREAD_LIST})
  target_link_libraries(mvnc "${MVNC_HIGHCLASS_LIB}")
else()
  add_library(mvnc SHARED ${MVNC_SRC_LIST} ${XLINK_PC_LIST} ${XLINK_SHARED_LIST} ${XLINKCONSOLE_PC_LIST} ${USB_WIN_LIST} ${WIN_PTHREAD_LIST})
  target_link_libraries(mvnc mvnc_highclass)
endif()


set_target_output_dirs(mvnc   lib lib bin)
#INSTALL
install(DIRECTORY ${CMAKE_BINARY_DIR}/build/ DESTINATION .
  FILES_MATCHING PATTERN "*"
  PATTERN "*.ilk" EXCLUDE
  PATTERN "*.pdb" EXCLUDE
  PATTERN "*.exp" EXCLUDE)

file(GLOB MVNC_HEADER_FILES "${CMAKE_SOURCE_DIR}/include/mvnc.h" "${CMAKE_SOURCE_DIR}/include/ncHighClass.h") 
install(FILES ${MVNC_HEADER_FILES} DESTINATION include)



