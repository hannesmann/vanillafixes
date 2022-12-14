cmake_minimum_required(VERSION 3.16...3.20)

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
add_subdirectory(src)