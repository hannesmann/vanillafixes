add_library(vanillafixes_dll SHARED
	"dll.c"
	"tsc.c"

	"${CMAKE_SOURCE_DIR}/src/loader.c"
	"${CMAKE_SOURCE_DIR}/src/os.c"
	"${CMAKE_SOURCE_DIR}/src/textfile.c")

target_compile_features(vanillafixes_dll PRIVATE c_std_99)

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(vanillafixes_dll PRIVATE "/W3")
	set_target_properties(vanillafixes_dll PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")
	target_link_options(vanillafixes_dll PRIVATE "/MANIFEST:NO")
else()
	# msse2 needed for _mm_lfence
	target_compile_options(vanillafixes_dll PRIVATE "-Wall" "-Wno-unused-variable" "-msse2" "-municode" "-fno-strict-aliasing")
	target_link_options(vanillafixes_dll PRIVATE "-static-libgcc" "-municode")

	if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
		target_link_options(vanillafixes_dll PRIVATE "-s")
	endif()
endif()

target_compile_definitions(vanillafixes_dll PRIVATE -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS)
target_include_directories(vanillafixes_dll PRIVATE "${CMAKE_SOURCE_DIR}/src")

# Include MinHook in the DLL
target_include_directories(vanillafixes_dll PRIVATE "${CMAKE_SOURCE_DIR}/minhook/include")
target_sources(
	vanillafixes_dll PRIVATE

	"${CMAKE_SOURCE_DIR}/minhook/src/buffer.c"
	"${CMAKE_SOURCE_DIR}/minhook/src/hook.c"
	"${CMAKE_SOURCE_DIR}/minhook/src/trampoline.c"
	"${CMAKE_SOURCE_DIR}/minhook/src/hde/hde32.c"
)

target_link_libraries(vanillafixes_dll shlwapi winmm)

set_target_properties(vanillafixes_dll PROPERTIES PREFIX "")
set_target_properties(vanillafixes_dll PROPERTIES OUTPUT_NAME "VfPatcher")
set_target_properties(vanillafixes_dll PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
