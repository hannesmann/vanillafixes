add_executable(vanillafixes_launcher WIN32 "launcher.c" "launcher.rc")

target_compile_features(vanillafixes_launcher PRIVATE c_std_99)
set_target_properties(vanillafixes_launcher PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")

target_precompile_headers(vanillafixes_launcher PRIVATE "${CMAKE_SOURCE_DIR}/src/pch.h")

target_link_libraries(vanillafixes_launcher shlwapi)
target_link_options(vanillafixes_launcher PRIVATE "/MANIFEST:NO")

set_target_properties(vanillafixes_launcher PROPERTIES OUTPUT_NAME "VanillaFixes")
set_target_properties(vanillafixes_launcher PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")