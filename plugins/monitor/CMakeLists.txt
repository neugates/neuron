# neuron monitor plugin
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/plugins")

file(COPY ${CMAKE_SOURCE_DIR}/plugins/monitor/monitor.json DESTINATION ${CMAKE_BINARY_DIR}/plugins/schema/)

set(src
    monitor.c
    monitor_config.c
    mqtt_handle.c
)

add_library(plugin-monitor SHARED ${src})

target_include_directories(plugin-monitor PRIVATE
  ${CMAKE_SOURCE_DIR}/include/neuron
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(plugin-monitor neuron-base nng)
target_link_libraries(plugin-monitor ${CMAKE_THREAD_LIBS_INIT})
