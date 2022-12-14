set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_C_STANDARD 99)
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)

if(NOT CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	message(
		FATAL_ERROR
		"VanillaFixes can only be compiled with Microsoft Visual C++.\n"
		"If you're running Linux, use Wine with https://github.com/mstorsjo/msvc-wine."
	)
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
	message(
		FATAL_ERROR
		"64-bit environment detected. Launch CMake with the x86 VS Native Tools command prompt."
	)
endif()

add_executable(vanillafixes_launcher WIN32 "launcher.c" "res/launcher.rc")
add_library(vanillafixes_dll SHARED "dll.c")

target_link_libraries(vanillafixes_launcher shlwapi shell32)
target_link_libraries(vanillafixes_dll winmm)

# Include MinHook in the DLL
target_include_directories(vanillafixes_dll PRIVATE "${CMAKE_SOURCE_DIR}/minhook/include")

target_sources(
	vanillafixes_dll PRIVATE

	"${CMAKE_SOURCE_DIR}/minhook/src/buffer.c"
	"${CMAKE_SOURCE_DIR}/minhook/src/hook.c"
	"${CMAKE_SOURCE_DIR}/minhook/src/trampoline.c"
	"${CMAKE_SOURCE_DIR}/minhook/src/hde/hde32.c"
)

target_link_options(vanillafixes_launcher PRIVATE "/manifest:no")
target_link_options(vanillafixes_dll PRIVATE "/manifest:no")

set_target_properties(vanillafixes_launcher PROPERTIES OUTPUT_NAME "VanillaFixes")
set_target_properties(vanillafixes_dll PROPERTIES OUTPUT_NAME "VfPatcher")