aux_source_directory(. SRC_LIST)

if(MSVC)
function(SET_TARGET_OUTPUT_DIRS target dir_archive dir_library dir_runtime)
  if(${CMAKE_VERSION} VERSION_GREATER "3.0.1")
    set_target_properties(
      ${target}
      PROPERTIES
      ARCHIVE_OUTPUT_DIRECTORY                "${CMAKE_BINARY_DIR}/output/${dir_archive}"
      ARCHIVE_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/output/${dir_archive}"
      ARCHIVE_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/output/${dir_archive}"
      ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_BINARY_DIR}/output/${dir_archive}"
      ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/output/${dir_archive}"
      LIBRARY_OUTPUT_DIRECTORY                "${CMAKE_BINARY_DIR}/output/${dir_library}"
      LIBRARY_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/output/${dir_library}"
      LIBRARY_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/output/${dir_library}"
      LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_BINARY_DIR}/output/${dir_library}"
      LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/output/${dir_library}"
      RUNTIME_OUTPUT_DIRECTORY                "${CMAKE_BINARY_DIR}/output/${dir_runtime}"
      RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/output/${dir_runtime}"
      RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/output/${dir_runtime}"
      RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_BINARY_DIR}/output/${dir_runtime}"
      RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/output/${dir_runtime}"
    )
  endif()
endfunction()
endif()

if(MSVC)
    link_directories(${CMAKE_BINARY_DIR}/getopt9/lib)
    include_directories(${CMAKE_SOURCE_DIR})
    include_directories(${CMAKE_BINARY_DIR}/json-c/include)
    include_directories(${CMAKE_BINARY_DIR}/getopt9/include)
    list(REMOVE_ITEM SRC_LIST ./i2cbusses.c)
    list(REMOVE_ITEM SRC_LIST ./smbus_linux.c)
    message(STATUS "${SRC_LIST}")
    add_executable(bsl_reset ./bsl_reset.c)
    list(REMOVE_ITEM SRC_LIST ./bsl_reset.c)
    add_library(bsl STATIC ${SRC_LIST})
    add_dependencies(bsl json-c-static)
    target_link_libraries(bsl_reset bsl getopt)
    target_link_libraries(bsl  json-c-static)
    set_target_output_dirs(bsl_reset   lib lib bin)
    set_target_output_dirs(bsl   lib lib bin)
else()
    list(REMOVE_ITEM SRC_LIST ./smbus_win.c)
    add_library(bsl SHARED ${SRC_LIST})
    add_executable(bsl_reset ${SRC_LIST} ${UTILS_LIST})
    target_link_libraries(bsl_reset json-c)
endif()
target_link_libraries(bsl json-c)
set_target_properties(bsl PROPERTIES VERSION 0)


install(TARGETS bsl_reset DESTINATION bin)
install(TARGETS bsl DESTINATION lib)

file(GLOB FILES_TO_DEPLOY "../include/*")
install( FILES ${FILES_TO_DEPLOY} DESTINATION include )

if (NOT DEFINED INSTALL_USB_RULES)
  SET(INSTALL_USB_RULES OFF)
endif()

if (INSTALL_USB_RULES)
  if(UNIX)
    if(INSTALL_LOCALY)
      install(FILES 98-hddlbsl.rules  DESTINATION "${DESTDIR}/etc/udev/rules.d/")
    else()
      install(FILES 98-hddlbsl.rules  DESTINATION etc/udev/rules.d/)
    endif()
  endif()
endif()

if (TRIGGER_RULES)
  install(CODE "execute_process(COMMAND sudo modprobe i2c-i801)")
  install(CODE "execute_process(COMMAND udevadm control --reload-rules)")
  install(CODE "execute_process(COMMAND udevadm trigger)")
  install(CODE "execute_process(COMMAND ldconfig)")
endif()
