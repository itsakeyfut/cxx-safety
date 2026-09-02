//===----------------------------------------------------------------------===//
//
// Part of the cxx-safety project, under the Apache License v2.0 with LLVM
// Exceptions. See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef CXX_SAFETY_DRIVER_ASTDUMPACTION_H
#define CXX_SAFETY_DRIVER_ASTDUMPACTION_H

#include <memory>

#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

namespace cxx_safety {

/// Lists the function definition in each translation unit.
///
/// This is scaffolding for the analysis entry point: it exercises the path
/// from ClangTool to ASTContext without doing any analysis yet.
class ASTDumpAction : public clang::ASTFrontendAction {
public:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI,
            llvm::StringRef InFile) override;
};

std::unique_ptr<clang::tooling::FrontendActionFactory> newASTDumpActionFactory();

} // namespace cxx_safety

#endif // CXX_SAFETY_DRIVER_ASTDUMPACTION_H
