cmake_minimum_required (VERSION 2.6)

include_directories ("${PROJECT_SOURCE_DIR}/tracelib")

add_executable(trace_tool.bin trace_generator.cpp)

target_link_libraries (trace_tool.bin tracelib unicorn)

INSTALL(TARGETS trace_tool.bin RUNTIME DESTINATION bin)
