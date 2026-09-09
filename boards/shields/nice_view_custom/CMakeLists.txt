if(CONFIG_ZMK_DISPLAY AND CONFIG_NICE_VIEW_WIDGET_STATUS)
  set(EVA_ART_C ${CMAKE_CURRENT_BINARY_DIR}/eva_art.c)
  execute_process(
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/widgets/generate_art.py ${EVA_ART_C}
    RESULT_VARIABLE EVA_ART_RESULT
  )
  if(NOT EVA_ART_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate Evangelion nice!view art")
  endif()

  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
  zephyr_library_sources(custom_status_screen.c)
  zephyr_library_sources(widgets/bolt.c)
  zephyr_library_sources(widgets/util.c)
  zephyr_library_sources(${EVA_ART_C})
  zephyr_library_sources(widgets/peripheral_status.c)
endif()
