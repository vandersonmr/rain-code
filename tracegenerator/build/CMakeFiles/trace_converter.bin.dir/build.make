# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.7

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/vanderson/dev/mestrado/rain-code/tracegenerator

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/vanderson/dev/mestrado/rain-code/tracegenerator/build

# Include any dependencies generated for this target.
include CMakeFiles/trace_converter.bin.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/trace_converter.bin.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/trace_converter.bin.dir/flags.make

CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.o: CMakeFiles/trace_converter.bin.dir/flags.make
CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.o: ../trace_converter.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/vanderson/dev/mestrado/rain-code/tracegenerator/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.o -c /home/vanderson/dev/mestrado/rain-code/tracegenerator/trace_converter.cpp

CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/vanderson/dev/mestrado/rain-code/tracegenerator/trace_converter.cpp > CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.i

CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/vanderson/dev/mestrado/rain-code/tracegenerator/trace_converter.cpp -o CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.s

CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.o.requires:

.PHONY : CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.o.requires

CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.o.provides: CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.o.requires
	$(MAKE) -f CMakeFiles/trace_converter.bin.dir/build.make CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.o.provides.build
.PHONY : CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.o.provides

CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.o.provides.build: CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.o


# Object files for target trace_converter.bin
trace_converter_bin_OBJECTS = \
"CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.o"

# External object files for target trace_converter.bin
trace_converter_bin_EXTERNAL_OBJECTS =

trace_converter.bin: CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.o
trace_converter.bin: CMakeFiles/trace_converter.bin.dir/build.make
trace_converter.bin: CMakeFiles/trace_converter.bin.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/vanderson/dev/mestrado/rain-code/tracegenerator/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable trace_converter.bin"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/trace_converter.bin.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/trace_converter.bin.dir/build: trace_converter.bin

.PHONY : CMakeFiles/trace_converter.bin.dir/build

CMakeFiles/trace_converter.bin.dir/requires: CMakeFiles/trace_converter.bin.dir/trace_converter.cpp.o.requires

.PHONY : CMakeFiles/trace_converter.bin.dir/requires

CMakeFiles/trace_converter.bin.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/trace_converter.bin.dir/cmake_clean.cmake
.PHONY : CMakeFiles/trace_converter.bin.dir/clean

CMakeFiles/trace_converter.bin.dir/depend:
	cd /home/vanderson/dev/mestrado/rain-code/tracegenerator/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/vanderson/dev/mestrado/rain-code/tracegenerator /home/vanderson/dev/mestrado/rain-code/tracegenerator /home/vanderson/dev/mestrado/rain-code/tracegenerator/build /home/vanderson/dev/mestrado/rain-code/tracegenerator/build /home/vanderson/dev/mestrado/rain-code/tracegenerator/build/CMakeFiles/trace_converter.bin.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/trace_converter.bin.dir/depend

