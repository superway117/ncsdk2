aux_source_directory(./ SRC_LIST)

ADD_LIBRARY(ion SHARED ${SRC_LIST})
target_link_libraries(ion pthread m)
SET_TARGET_PROPERTIES(ion PROPERTIES VERSION 0)

install(TARGETS ion DESTINATION lib)

file(GLOB ION_HEADER_FILES "./*.h")
install(FILES ${ION_HEADER_FILES} DESTINATION include)

if(UNIX)
  if(INSTALL_LOCALY)
    install(FILES 99-hddl-ion.rules DESTINATION "${DESTDIR}/etc/udev/rules.d/")
  else()
    install(FILES 99-hddl-ion.rules DESTINATION etc/udev/rules.d/)
  endif()
endif()

if (INSTALL_LOCALY)
  install(CODE "execute_process(COMMAND udevadm control --reload-rules)")
  install(CODE "execute_process(COMMAND udevadm trigger)")
  install(CODE "execute_process(COMMAND ldconfig)")
endif()
