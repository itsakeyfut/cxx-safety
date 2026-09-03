//===----------------------------------------------------------------------===//
//
// Part of the cxx-safety project, under the Apache License v2.0 with LLVM
// Exceptions. See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef CXX_SAFETY_DRIVER_CFGDUMPACTION_H
#define CXX_SAFETY_DRIVER_CFGDUMPACTION_H

#include <memory>

#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

namespace cxx_safety {

/// Builds a CFG for every function definition in the main file and prints its
/// blocks and edges.
///
/// This is the shape every later analysis operates on, so the dump doubles as
/// a debugging tool once the data-flow framework exists.
class CFGDumpAction : public clang::ASTFrontendAction {
public:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI,
            llvm::StringRef InFile) override;
};

    std::unique_ptr<clang::tooling::FrontendActionFactory> newCFGDumpActionFactory();

}  // namespace cxx_safety

#endif // CXX_SAFETY_DRIVER_CFGDUMPACTION_H
