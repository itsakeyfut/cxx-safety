//===----------------------------------------------------------------------===//
//
// Part of the cxx-safety project, under the Apache License v2.0 with LLVM
// Exceptions. See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/Basic/Version.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/Support/raw_ostream.h"

namespace {

constexpr const char *kVersion = "0.1.0";

} // namespace

int main() {
    // LLVM_VERSION_STRING resolves at compile time from a header, while
    // getClangFullVersion() requires linkingg against clangBasic. Printing both
    // verifies that headers and libraries are wired up correctly.
    llvm::outs() << "cxx-safety " << kVersion << "\n";
    llvm::outs() << "  LLVM  : " << LLVM_VERSION_STRING << "\n";
    llvm::outs() << "  Clang : " << clang::getClangFullVersion() << "\n";
    return 0;
}
