//===----------------------------------------------------------------------===//
//
// Part of the cxx-safety project, under the Apache License v2.0 with LLVM
// Exceptions. See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "cxx-safety/Driver/ASTDumpAction.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/raw_ostream.h"

namespace cxx_safety {

namespace {

class FunctionLister : public clang::RecursiveASTVisitor<FunctionLister> {
public:
    explicit FunctionLister(clang::ASTContext &Context): Context(Context) {}

    bool VisitFunctionDecl(clang::FunctionDecl *FD) {
        if (!FD->hasBody() || !FD->isThisDeclarationADefinition())
            return true;

        // Declaration pulled in from headers are not what we want to analyze.
        const clang::SourceManager &SM = Context.getSourceManager();
        if (!SM.isInMainFile(FD->getLocation()))
            return true;

        llvm::outs() << "    " << FD->getQualifiedNameAsString() << " at "
                << FD->getLocation().printToString(SM) << "\n";
        ++Count;
        return true;
    }

    unsigned getCount() const { return Count; }

private:
    clang::ASTContext &Context;
    unsigned Count = 0;
};

class FunctionListerConsumer : public clang::ASTConsumer {
public:
    void HandleTranslationUnit(clang::ASTContext &Context) override {
        const clang::SourceManager &SM = Context.getSourceManager();
        llvm::outs() << "cxx-safety: " << SM.getFileEntryRefForID(SM.getMainFileID())->getName() << "\n";

        FunctionLister Lister(Context);
        Lister.TraverseDecl(Context.getTranslationUnitDecl());

        llvm::outs() << "  functions: " << Lister.getCount() << "\n";
    }
};

}  // namespace

std::unique_ptr<clang::ASTConsumer> ASTDumpAction::CreateASTConsumer(clang::CompilerInstance &,
        llvm::StringRef) {
    return std::make_unique<FunctionListerConsumer>();
}

std::unique_ptr<clang::tooling::FrontendActionFactory> newASTDumpActionFactory() {
    return clang::tooling::newFrontendActionFactory<ASTDumpAction>();
}

}  // namespace cxx_safety
