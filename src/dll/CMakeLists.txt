add_library(vanillafixes_dll SHARED "dll.c")

target_compile_features(vanillafixes_dll PRIVATE c_std_99)
set_target_properties(vanillafixes_dll PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")

target_compile_definitions(vanillafixes_dll PRIVATE -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN)
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

target_link_libraries(vanillafixes_dll winmm)
target_link_options(vanillafixes_dll PRIVATE "/MANIFEST:NO")

set_target_properties(vanillafixes_dll PROPERTIES OUTPUT_NAME "VfPatcher")
set_target_properties(vanillafixes_dll PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")