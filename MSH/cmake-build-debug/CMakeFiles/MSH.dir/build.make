# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.14

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
CMAKE_COMMAND = /snap/clion/83/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /snap/clion/83/bin/cmake/linux/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/anthony/Desktop/School/Fall2019/OS/MSH

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/anthony/Desktop/School/Fall2019/OS/MSH/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/MSH.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/MSH.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/MSH.dir/flags.make

CMakeFiles/MSH.dir/msh.c.o: CMakeFiles/MSH.dir/flags.make
CMakeFiles/MSH.dir/msh.c.o: ../msh.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/anthony/Desktop/School/Fall2019/OS/MSH/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/MSH.dir/msh.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/MSH.dir/msh.c.o   -c /home/anthony/Desktop/School/Fall2019/OS/MSH/msh.c

CMakeFiles/MSH.dir/msh.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/MSH.dir/msh.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/anthony/Desktop/School/Fall2019/OS/MSH/msh.c > CMakeFiles/MSH.dir/msh.c.i

CMakeFiles/MSH.dir/msh.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/MSH.dir/msh.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/anthony/Desktop/School/Fall2019/OS/MSH/msh.c -o CMakeFiles/MSH.dir/msh.c.s

# Object files for target MSH
MSH_OBJECTS = \
"CMakeFiles/MSH.dir/msh.c.o"

# External object files for target MSH
MSH_EXTERNAL_OBJECTS =

MSH: CMakeFiles/MSH.dir/msh.c.o
MSH: CMakeFiles/MSH.dir/build.make
MSH: CMakeFiles/MSH.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/anthony/Desktop/School/Fall2019/OS/MSH/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable MSH"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/MSH.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/MSH.dir/build: MSH

.PHONY : CMakeFiles/MSH.dir/build

CMakeFiles/MSH.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/MSH.dir/cmake_clean.cmake
.PHONY : CMakeFiles/MSH.dir/clean

CMakeFiles/MSH.dir/depend:
	cd /home/anthony/Desktop/School/Fall2019/OS/MSH/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/anthony/Desktop/School/Fall2019/OS/MSH /home/anthony/Desktop/School/Fall2019/OS/MSH /home/anthony/Desktop/School/Fall2019/OS/MSH/cmake-build-debug /home/anthony/Desktop/School/Fall2019/OS/MSH/cmake-build-debug /home/anthony/Desktop/School/Fall2019/OS/MSH/cmake-build-debug/CMakeFiles/MSH.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/MSH.dir/depend
