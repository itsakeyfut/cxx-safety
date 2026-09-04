//===----------------------------------------------------------------------===//
//
// Part of the cxx-safety project, under the Apache License v2.0 with LLVM
// Exceptions. See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "cxx-safety/Driver/CFGDumpAction.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/Analysis/CFG.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/raw_ostream.h"

namespace cxx_safety {

namespace {

/// Names the variable a CFG element refers to, or "<unknown>" when the element
/// carries no declaration.
llvm::StringRef getVarName(const clang::VarDecl *VD) {
    return VD ? VD->getName() : "<unknown>";
}

/// Produces a readable label for a function, distinguishing lambda call
/// operators from named functions.
std::string getFunctionLabel(const clang::FunctionDecl *FD) {
    if (const auto *Method = llvm::dyn_cast<clang::CXXMethodDecl>(FD))
        if (Method->getParent()->isLambda())
            return "lambda operator()";
    return FD->getQualifiedNameAsString();
}

/// Prints the elements a CFG carries besides statements: object destruction,
/// member initialization, and scope boundaries.
///
/// These are implicit in the source, which is why they need spelling out here.
bool printNonStmtElement(const clang::CFGElement &Element, llvm::raw_ostream & OS) {
    if (std::optional<clang::CFGAutomaticObjDtor> Dtor = Element.getAs<clang::CFGAutomaticObjDtor>()) {
        OS << "~" << getVarName(Dtor->getVarDecl()) << "() (automatic)\n";
        return true;
    }
    if (Element.getAs<clang::CFGTemporaryDtor>()) {
        OS << "~temporary()\n";
        return true;
    }
    if (std::optional<clang::CFGInitializer> Init = Element.getAs<clang::CFGInitializer>()) {
        const clang::CXXCtorInitializer *CI = Init->getInitializer();
        if (const clang::FieldDecl *Field = CI->getAnyMember())
            OS << "init " << Field->getName() << "\n";
        else
            OS << "init <base>\n";
        return true;
    }
    if (std::optional<clang::CFGLifetimeEnds> Ends = Element.getAs<clang::CFGLifetimeEnds>()) {
        OS << "lifetime ends: " << getVarName(Ends->getVarDecl()) << "\n";
        return true;
    }
    if (std::optional<clang::CFGScopeBegin> Begin = Element.getAs<clang::CFGScopeBegin>()) {
        OS << "scope begin: " << getVarName(Begin->getVarDecl()) << "\n";
        return true;
    }
    if (std::optional<clang::CFGScopeEnd> End = Element.getAs<clang::CFGScopeEnd>()) {
        OS << "scope end: " << getVarName(End->getVarDecl()) << "\n";
        return true;
    }
    return false;
}

void printElement(const clang::CFGElement &Element, const clang::ASTContext &Context, llvm::raw_ostream &OS) {
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

    if (printNonStmtElement(Element, OS))
        return;

    OS << "<kind " << Element.getKind() << ">\n";
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

    /// Descends into a lambda body.
    ///
    /// The closure type and its call operator are implicit, so the default
    /// traversal never reaches them and the body collapses into a single
    /// expression, Enabling shouldVisitImplicitCode would reach them, but also
    /// every compiler-generated copy constructor and assignment operator.
    bool TraverseLambdaExpr(clang::LambdaExpr *LE) {
        if (!clang::RecursiveASTVisitor<CFGDumper>::TraverseLambdaExpr(LE))
            return false;

        if (clang::CXXMethodDecl *Call = LE->getCallOperator())
            dumpFunction(Call);
        return true;
    }

    bool VisitFunctionDecl(clang::FunctionDecl*FD) {
        if (!FD->hasBody() || !FD->isThisDeclarationADefinition())
            return true;

        const clang::SourceManager &SM = Context.getSourceManager();
        if (!SM.isInMainFile(FD->getLocation()))
            return true;

        dumpFunction(FD);
        return true;
    }

private:
    void dumpFunction(const clang::FunctionDecl *FD) {
        clang::CFG::BuildOptions Options;
        // Without this the builder folds subexpressions into their parent statement.
        // Data-flow analysis needs each read and write as its own element, so every statement is added.
        Options.setAllAlwaysAdd();

        // Object creation and destruction are implicit in the source but explicit
        // in the CFG, and every validity analysis is built on them.
        Options.AddInitializers = true;
        Options.AddImplicitDtors = true;
        Options.AddTemporaryDtors = true;

        // Lifetime markers cover non-class types, which have no destructor to
        // observe. Detecting a use of an `int` after its scope ends depends on
        // them.
        Options.AddLifetime = true;

        // Scope boundaries are what lifetime markers are anchored to.
        Options.AddScopes = true;

        std::unique_ptr<clang::CFG> Graph = clang::CFG::buildCFG(FD, FD->getBody(), &Context, Options);
        if (!Graph) {
            llvm::errs() << "warning: could not build a CFG for '" << FD->getQualifiedNameAsString() << "'\n";
            return;
        }

        const clang:: SourceManager &SM = Context.getSourceManager();
        llvm::outs() << "\nfunction '" << getFunctionLabel(FD) << "' at " << FD->getLocation().printToString(SM) << "\n";

        // Iterating the CFG directly yields blocks in an unspecified order.
        // Reverse post-order is what a forward data-flow analysis wants, and it
        // also reads top to bottom.
        for (const clang::CFGBlock *Block: llvm::reverse(*Graph))
            printBlock(*Block, *Graph, Context, llvm::outs());
    }

    clang::ASTContext & Context;
};

class CFGDumpConsumer : public clang::ASTConsumer {
public:
    void HandleTranslationUnit(clang::ASTContext &Context) override {
        const clang::SourceManager &SM = Context.getSourceManager();

        // The main file has no FileEntry when the input comes from stdin or an
        // in-memory buffer, which happens in tests and editor integrations.
        llvm::StringRef Name = "<input>";
        if (auto File = SM.getFileEntryRefForID(SM.getMainFileID()))
            Name = File->getName();

        llvm::outs() << "cxx-safety: " << Name << "\n";

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
