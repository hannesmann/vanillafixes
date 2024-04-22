add_executable(vanillafixes_launcher WIN32
	"main.c"
	"cmdline.c"
	"selftest.c"

	"${CMAKE_SOURCE_DIR}/src/loader.c"
	"${CMAKE_SOURCE_DIR}/src/os.c"
	"${CMAKE_SOURCE_DIR}/src/textfile.c"

	"launcher.rc")

target_compile_features(vanillafixes_launcher PRIVATE c_std_99)
target_compile_options(vanillafixes_launcher PRIVATE "/W3")
set_target_properties(vanillafixes_launcher PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")

target_compile_definitions(vanillafixes_launcher PRIVATE -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS)
target_include_directories(vanillafixes_launcher PRIVATE "${CMAKE_SOURCE_DIR}/src")

target_link_libraries(vanillafixes_launcher shlwapi winmm)
target_link_options(vanillafixes_launcher PRIVATE "/MANIFEST:NO")

set_target_properties(vanillafixes_launcher PROPERTIES OUTPUT_NAME "VanillaFixes")
set_target_properties(vanillafixes_launcher PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
