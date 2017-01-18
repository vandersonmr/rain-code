cmake_minimum_required (VERSION 2.6)

include_directories ("${PROJECT_SOURCE_DIR}/tracelib")
include_directories ("${PROJECT_SOURCE_DIR}/")

add_executable(trace_tool.bin trace_generator.cpp)
add_executable(trace_converter.bin trace_converter.cpp)

target_link_libraries (trace_tool.bin tracelib unicorn)

target_link_libraries (trace_converter.bin tracelib)

INSTALL(TARGETS trace_tool.bin trace_converter.bin RUNTIME DESTINATION bin)
