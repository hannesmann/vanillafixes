# This line sets the minimum required version of CMake for this project to be 3.16 and maximum to be 3.26.
# This is done to ensure that the project will be built using a version of CMake that the project has been tested with.
cmake_minimum_required(VERSION 3.16...3.26)

# Setting the default build type for the project to "Release".
# The build type determines the level of optimization and debugging information for the built files.
# If no build type is specified by the user, it will default to "Release".
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "Setting build type to 'Release' as none was specified.")
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)

	# This line sets the possible build types that can be chosen in the CMake GUI.
	# This allows the user to easily switch between different build types.
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
else()
	message(STATUS "Build type is set to '${CMAKE_BUILD_TYPE}'.")
endif()

# If the generator is Visual Studio, this sets the platform to Win32.
# This is done because the project is intended to be built as a 32-bit application.
if(CMAKE_GENERATOR MATCHES "Visual Studio")
	set(CMAKE_GENERATOR_PLATFORM Win32)
endif()

# This line sets the project name to "vanillafixes" and specifies that the project is written in the C language.
project(vanillafixes LANGUAGES C)

# This checks that the C compiler is Microsoft Visual C++ (MSVC).
# The project is intended to be built with MSVC, and may not compile correctly with other compilers.
if(NOT CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	message(
		FATAL_ERROR
		"VanillaFixes can only be compiled with Microsoft Visual C++.\n"
		"If you're running Linux, use Wine with https://github.com/mstorsjo/msvc-wine."
	)
endif()

# This checks that the project is being built in a 32-bit environment.
# The project is intended to be built as a 32-bit application, and may not compile or run correctly in a 64-bit environment.
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
	message(
		FATAL_ERROR
		"64-bit environment detected. Launch CMake with the x86 VS Native Tools command prompt."
	)
endif()

# These lines define options to build the DLL and the launcher.
# They can be set by the user when configuring the project.
# By default, both the DLL and the launcher will be built.
set(BUILD_DLL 1 CACHE STRING "Build the DLL (VfPatcher.dll)")
set(BUILD_LAUNCHER 1 CACHE STRING "Build the launcher (VanillaFixes.exe)")

# If the BUILD_DLL option is enabled, the subdirectory "src/dll" is added to the build.
# This directory should contain the source files for the DLL.
if(BUILD_DLL)
	add_subdirectory(src/dll)
endif()

# If the BUILD_LAUNCHER option is enabled, the subdirectory "src/launcher" is added to the build.
# This directory should contain the source files for the launcher.
if(BUILD_LAUNCHER)
	add_subdirectory(src/launcher)
endif()
