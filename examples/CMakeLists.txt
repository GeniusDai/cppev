function(compile_target target_name_)
    add_executable(${target_name_} ${target_name_}.cc)
    target_link_libraries(${target_name_} cppev)
    target_compile_options(${target_name_}  PRIVATE $<TARGET_PROPERTY:cppev,INTERFACE_COMPILE_OPTIONS>)
endfunction(compile_target)

add_subdirectory(tcp_stress)
add_subdirectory(file_transfer)
add_subdirectory(io_evlp)
