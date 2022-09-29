function(compile_target target_name_)
    add_executable(${target_name_} ${target_name_}.cc)
    target_link_libraries(${target_name_} cppev)
endfunction(compile_target)

add_subdirectory(tcp_stress)
add_subdirectory(file_transfer)
add_subdirectory(nio_evlp)
