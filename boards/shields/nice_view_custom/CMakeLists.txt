if(CONFIG_ZMK_DISPLAY AND CONFIG_NICE_VIEW_WIDGET_STATUS)
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
  zephyr_library_sources(custom_status_screen.c)
  zephyr_library_sources(widgets/bolt.c)
  zephyr_library_sources(widgets/util.c)

  if(NOT CONFIG_ZMK_SPLIT OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    zephyr_library_sources(widgets/nerv_static_art.c)
    zephyr_library_sources(widgets/status_prerender.c)
  else()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(EVA_ART_C ${CMAKE_CURRENT_BINARY_DIR}/eva_art.c)
    execute_process(
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/widgets/generate_art.py ${EVA_ART_C}
      RESULT_VARIABLE EVA_ART_RESULT
    )
    if(NOT EVA_ART_RESULT EQUAL 0)
      message(FATAL_ERROR "Failed to generate Evangelion nice!view art")
    endif()
    zephyr_library_sources(${EVA_ART_C})
    zephyr_library_sources(widgets/peripheral_status.c)
  endif()
endif()
