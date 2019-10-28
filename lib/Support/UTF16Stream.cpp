/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "hermes/Support/UTF16Stream.h"

#include <cstdlib>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/ConvertUTF.h"
#include "llvm/Support/MathExtras.h"

namespace hermes {

/// Number of char16_t in the internal conversion buffer (if UTF8 input).
static constexpr size_t kChunkChars = 1024;

UTF16Stream::UTF16Stream(llvm::ArrayRef<uint8_t> utf8)
    : utf8Begin_(utf8.begin()), utf8End_(utf8.end()), storage_(kChunkChars) {
  // Exhaust the buffer and refill it.
  cur_ = end_ = storage_.end();
}

bool UTF16Stream::refill() {
  assert(cur_ == end_ && "cannot refill when data remains");
  if (utf8Begin_ == utf8End_) {
    // Pass-through mode, or final chunk already converted.
    // Either way, there's nothing (more) to convert.
    return false;
  }
  assert(utf8Begin_ && utf8End_ && "must have input to convert");

  // Reset the conversion buffer.
  cur_ = storage_.begin();
  end_ = storage_.end();
  auto out = storage_.begin();

  // Fast case for any ASCII prefix...
  while (out != end_ && utf8Begin_ != utf8End_ && *utf8Begin_ < 128) {
    *out++ = *utf8Begin_++;
  }
  // ...and call the library for any non-ASCII remainder. Conversion always
  // stops at a code point boundary.
  llvm::ConversionResult cRes = ConvertUTF8toUTF16(
      &utf8Begin_,
      utf8End_,
      (llvm::UTF16 **)&out,
      (llvm::UTF16 *)end_,
      llvm::lenientConversion);

  if (cRes != llvm::ConversionResult::targetExhausted) {
    // Indicate that we have converted the final chunk.
    utf8Begin_ = utf8End_;
  }
  end_ = out;

  // Did we actually convert anything?
  return cur_ != end_;
}

} // namespace hermes
