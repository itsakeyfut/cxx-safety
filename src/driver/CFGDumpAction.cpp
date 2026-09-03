//===----------------------------------------------------------------------===//
//
// Part of the cxx-safety project, under the Apache License v2.0 with LLVM
// Exceptions. See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "cxx-safety/Driver/CFGDumpAction.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/Analysis/CFG.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/raw_ostream.h"

namespace cxx_safety {

namespace {

void printElement(const clang::CFGElement &Element, const clang::ASTContext &Context, llvm::raw_ostream &OS) {
    // A CFG holds more than statements: implicit destructor calls, lifetime markers, and so on.
    // Only statements matter for now, but the others become relevant once destruction is tracked.
    if (std::optional<clang::CFGStmt> Stmt = Element.getAs<clang::CFGStmt>()) {
        const clang::Stmt *S = Stmt->getStmt();
        S->printPretty(OS, nullptr, clang::PrintingPolicy(Context.getLangOpts()));
        // printPretty omits implicit casts, which would make an lvalue and the read of its value print identically.
        // The distinction matters for tracking accesses, so the cast kind is spelled out.
        if (const auto *Cast = llvm::dyn_cast<clang::ImplicitCastExpr>(S))
            OS << " (" << Cast->getCastKindName() << ")";
        if (llvm::isa<clang::Expr>(S))
            OS << "\n";
        return;
    }
    OS << "<" << Element.getKind() << ">\n";
}

void printBlock(const clang::CFGBlock &Block, const clang::CFG &Graph, const clang::ASTContext &Context,
        llvm::raw_ostream &OS) {
    OS << "  [B" << Block.getBlockID();
    if (&Block == &Graph.getEntry())
        OS << " (ENTRY)";
    else if (&Block == &Graph.getExit())
        OS << " (EXIT)";
    OS << "]\n";

    unsigned Index = 0;
    for (const clang::CFGElement &Element: Block) {
        OS << "    " << Index++ << ": ";
        printElement(Element, Context, OS);
    }

    // The terminator statement spans the whole construct, so only its kind and
    // condition are printed; the condition itself is already an element above.
    if (const clang::Stmt *Terminator = Block.getTerminatorStmt()) {
        OS << "    T: " << Terminator->getStmtClassName();
        if (const clang::Stmt *Cond = Block.getTerminatorCondition()) {
            OS << " ";
            Cond->printPretty(OS, nullptr, clang::PrintingPolicy(Context.getLangOpts()));
        }
        OS << "\n";
    }

    if (Block.succ_empty())
        return;

    OS << "    succs:";
    for (const clang::CFGBlock::AdjacentBlock &Succ: Block.succs()) {
        // Unreachable successors are represented by null edges.
        if (Succ)
            OS << " B" << Succ->getBlockID();
        else OS << " <unreachable>";
    }
    OS << "\n";
}

class CFGDumper : public clang::RecursiveASTVisitor<CFGDumper> {
public:
    explicit CFGDumper(clang::ASTContext &Context): Context(Context) {}

    bool VisitFunctionDecl(clang::FunctionDecl*FD) {
        if (!FD->hasBody() || !FD->isThisDeclarationADefinition())
            return true;

        const clang::SourceManager &SM = Context.getSourceManager();
        if (!SM.isInMainFile(FD->getLocation()))
            return true;

        clang::CFG::BuildOptions Options;
        // Without this the builder folds subexpressions into their parent statement.
        // Data-flow analysis needs each read and write as its own element, so every statement is added.
        Options.setAllAlwaysAdd();
        std::unique_ptr<clang::CFG> Graph = clang::CFG::buildCFG(FD, FD->getBody(), &Context, Options);
        if (!Graph) {
            llvm::errs() << "warning: could not build a CFG for '" << FD->getQualifiedNameAsString() << "\n";
            return true;
        }

        llvm::outs() << "\nfunction '" << FD->getQualifiedNameAsString() << "' at "
                << FD->getLocation().printToString(SM) << "\n";

        // Iterating the CFG directly yields blocks in an unspecified order.
        // Reverse post-order is what a forward data-flow analysis wants, and it
        // also reads top to bottom.
        for (const clang::CFGBlock *Block: llvm::reverse(*Graph))
            printBlock(*Block, *Graph, Context, llvm::outs());

        return true;
    }

private:
    clang::ASTContext & Context;
};

class CFGDumpConsumer : public clang::ASTConsumer {
public:
    void HandleTranslationUnit(clang::ASTContext &Context) override {
        const clang::SourceManager &SM = Context.getSourceManager();
        llvm::outs() << "cxx-safety: " << SM.getFileEntryRefForID(SM.getMainFileID())->getName() << "\n";

        CFGDumper Dumper(Context);
        Dumper.TraverseDecl(Context.getTranslationUnitDecl());
    }
};

}  // namespace

std::unique_ptr<clang::ASTConsumer> CFGDumpAction::CreateASTConsumer(clang::CompilerInstance &, llvm::StringRef) {
    return std::make_unique<CFGDumpConsumer>();
}

std::unique_ptr<clang::tooling::FrontendActionFactory> newCFGDumpActionFactory() {
    return clang::tooling::newFrontendActionFactory<CFGDumpAction>();
}

}  // namespace cxx_safety
