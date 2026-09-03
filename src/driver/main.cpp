//===----------------------------------------------------------------------===//
//
// Part of the cxx-safety project, under the Apache License v2.0 with LLVM
// Exceptions. See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "cxx-safety/Driver/CFGDumpAction.h"

#include "clang/Basic/Version.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

namespace {

constexpr const char *kVersion = "0.1.0";

llvm::cl::OptionCategory CxxSafetyCategory("cxx-safety options");

const char *const kHelpText = "Analyzes C/C++ sources for memory and thread safety violations.\n";

void printVersion(llvm::raw_ostream &OS) {
    OS << "cxx-safety " << kVersion << "\n";
    OS << "  LLVM  : " << LLVM_VERSION_STRING << "\n";
    OS << "  Clang : " << clang::getClangFullVersion() << "\n";
}

} // namespace

int main(int argc, const char **argv) {
    llvm::cl::SetVersionPrinter(printVersion);

    llvm::Expected<clang::tooling::CommonOptionsParser> OptionsParser =
            clang::tooling::CommonOptionsParser::create(argc, argv, CxxSafetyCategory,
                    llvm::cl::OneOrMore, kHelpText);
    if (!OptionsParser) {
        llvm::errs() << llvm::toString(OptionsParser.takeError());
        return 1;
    }

    clang::tooling::ClangTool Tool(OptionsParser->getCompilations(),
            OptionsParser->getSourcePathList());

    return Tool.run(cxx_safety::newCFGDumpActionFactory().get());
}
