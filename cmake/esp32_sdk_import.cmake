
function(add_asm_compile_options)
    foreach(option ${ARGV})
        # note: the Visual Studio Generator doesn't support this...
        add_compile_options($<$<COMPILE_LANGUAGE:ASM>:${option}>)
    endforeach()
endfunction()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include($ENV{IDF_PATH}/tools/cmake/utilities.cmake)
# Use generator expression so that users can append/override flags even after call to
# idf_build_process
idf_build_get_property(include_directories INCLUDE_DIRECTORIES GENERATOR_EXPRESSION)
idf_build_get_property(compile_options COMPILE_OPTIONS GENERATOR_EXPRESSION)
idf_build_get_property(c_compile_options C_COMPILE_OPTIONS GENERATOR_EXPRESSION)
idf_build_get_property(cxx_compile_options CXX_COMPILE_OPTIONS GENERATOR_EXPRESSION)
idf_build_get_property(asm_compile_options ASM_COMPILE_OPTIONS GENERATOR_EXPRESSION)
idf_build_get_property(compile_definitions COMPILE_DEFINITIONS GENERATOR_EXPRESSION)
idf_build_get_property(common_reqs ___COMPONENT_REQUIRES_COMMON)

include_directories("${include_directories}")
add_compile_options("${compile_options}")
add_c_compile_options("${c_compile_options}")
add_cxx_compile_options("${cxx_compile_options}")
add_asm_compile_options("${asm_compile_options}")
add_compile_options("${compile_definitions}")
link_libraries(${common_reqs})
