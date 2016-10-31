#**************************************************************************
#   Copyright (C) 2013 by:                                                *
#   Edson Borin (edson@ic.unicamp.br)                                     *
#   Vanderson Rosario (vandersonmr2@gmail.com)                            *
#                                                                         *
#   This program is free software; you can redistribute it and/or modify  *
#   it under the terms of the GNU General Public License as published by  *
#   the Free Software Foundation; either version 2 of the License, or     *
#   (at your option) any later version.                                   *
#                                                                         *
#   This program is distributed in the hope that it will be useful,       *
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#   GNU General Public License for more details.                          *
#                                                                         *
#   You should have received a copy of the GNU General Public License     *
#   along with this program; if not, write to the                         *
#   Free Software Foundation, Inc.,                                       *
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
#**************************************************************************
cmake_minimum_required (VERSION 2.6)

project (RAIn)

set(CMAKE_CXX_FLAGS "-std=c++11 -g -O2")

# Add libraries
add_subdirectory(arglib)
add_subdirectory(tracelib)
add_subdirectory(rainlib)
add_subdirectory(tracegenerator)

# Add executable
add_executable(rain_tool.bin main.cpp)
add_executable(filter_tool.bin filter.cpp)

include_directories ("${PROJECT_SOURCE_DIR}/arglib")
include_directories ("${PROJECT_SOURCE_DIR}/tracelib")
include_directories ("${PROJECT_SOURCE_DIR}/rainlib")
include_directories ("${PROJECT_SOURCE_DIR}")

target_link_libraries (rain_tool.bin arglib tracelib rainlib udis86)
target_link_libraries (filter_tool.bin arglib tracelib)

INSTALL(TARGETS rain_tool.bin filter_tool.bin RUNTIME DESTINATION bin)
