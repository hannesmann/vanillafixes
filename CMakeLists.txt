cmake_minimum_required(VERSION 3.16...3.25)

# Set the default build type to "Release"
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "Setting build type to 'Release' as none was specified.")
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)

	# Set the possible values of build type for cmake-gui
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
else()
	message(STATUS "Build type is set to '${CMAKE_BUILD_TYPE}'.")
endif()

project(vanillafixes LANGUAGES C)

# Check for Microsoft Visual C++
if(NOT CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	message(
		FATAL_ERROR
		"VanillaFixes can only be compiled with Microsoft Visual C++.\n"
		"If you're running Linux, use Wine with https://github.com/mstorsjo/msvc-wine."
	)
endif()

# Check for 32-bit environment
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
	message(
		FATAL_ERROR
		"64-bit environment detected. Launch CMake with the x86 VS Native Tools command prompt."
	)
endif()

set(BUILD_DLL 1 CACHE STRING "Build the DLL (VfPatcher.dll)")
set(BUILD_LAUNCHER 1 CACHE STRING "Build the launcher (VanillaFixes.exe)")

if(BUILD_DLL)
	add_subdirectory(src/dll)
endif()
if(BUILD_LAUNCHER)
	add_subdirectory(src/launcher)
endif()