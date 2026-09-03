add_executable(cxx-safety
        CFGDumpAction.cpp
        main.cpp
)

llvm_map_components_to_libnames(cxx_safety_llvm_libs
        core
        support)

target_link_libraries(cxx-safety
        PRIVATE
        cxx_safety_options
        clangAnalysis
        clangAST
        clangBasic
        clangFrontend
        clangTooling
        ${cxx_safety_llvm_libs})
