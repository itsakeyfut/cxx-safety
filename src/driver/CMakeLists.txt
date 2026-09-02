add_executable(cxx-safety
        main.cpp)

llvm_map_components_to_libnames(cxx_safety_llvm_libs
        support)

target_link_libraries(cxx-safety
        PRIVATE
        cxx_safety_options
        clangBasic
        ${cxx_safety_llvm_libs})
