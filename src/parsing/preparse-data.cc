// Copyright 2010 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/parsing/preparse-data.h"
#include "src/base/hashmap.h"
#include "src/base/logging.h"
#include "src/globals.h"
#include "src/parsing/parser.h"
#include "src/parsing/preparse-data-format.h"

namespace v8 {
namespace internal {

void ParserLogger::LogFunction(int start, int end, int num_parameters,
                               int function_length,
                               bool has_duplicate_parameters, int literals,
                               int properties, LanguageMode language_mode,
                               bool uses_super_property, bool calls_eval) {
  function_store_.Add(start);
  function_store_.Add(end);
  function_store_.Add(num_parameters);
  function_store_.Add(function_length);
  function_store_.Add(literals);
  function_store_.Add(properties);
  function_store_.Add(
      FunctionEntry::EncodeFlags(language_mode, uses_super_property, calls_eval,
                                 has_duplicate_parameters));
}

ParserLogger::ParserLogger() {
  preamble_[PreparseDataConstants::kMagicOffset] =
      PreparseDataConstants::kMagicNumber;
  preamble_[PreparseDataConstants::kVersionOffset] =
      PreparseDataConstants::kCurrentVersion;
  preamble_[PreparseDataConstants::kHasErrorOffset] = false;
  preamble_[PreparseDataConstants::kFunctionsSizeOffset] = 0;
  preamble_[PreparseDataConstants::kSizeOffset] = 0;
  DCHECK_EQ(5, PreparseDataConstants::kHeaderSize);
#ifdef DEBUG
  prev_start_ = -1;
#endif
}

void ParserLogger::LogMessage(int start_pos, int end_pos,
                              MessageTemplate::Template message,
                              const char* arg_opt, ParseErrorType error_type) {
  if (HasError()) return;
  preamble_[PreparseDataConstants::kHasErrorOffset] = true;
  function_store_.Reset();
  STATIC_ASSERT(PreparseDataConstants::kMessageStartPos == 0);
  function_store_.Add(start_pos);
  STATIC_ASSERT(PreparseDataConstants::kMessageEndPos == 1);
  function_store_.Add(end_pos);
  STATIC_ASSERT(PreparseDataConstants::kMessageArgCountPos == 2);
  function_store_.Add((arg_opt == NULL) ? 0 : 1);
  STATIC_ASSERT(PreparseDataConstants::kParseErrorTypePos == 3);
  function_store_.Add(error_type);
  STATIC_ASSERT(PreparseDataConstants::kMessageTemplatePos == 4);
  function_store_.Add(static_cast<unsigned>(message));
  STATIC_ASSERT(PreparseDataConstants::kMessageArgPos == 5);
  if (arg_opt != NULL) WriteString(CStrVector(arg_opt));
}

void ParserLogger::WriteString(Vector<const char> str) {
  function_store_.Add(str.length());
  for (int i = 0; i < str.length(); i++) {
    function_store_.Add(str[i]);
  }
}

ScriptData* ParserLogger::GetScriptData() {
  int function_size = function_store_.size();
  int total_size = PreparseDataConstants::kHeaderSize + function_size;
  unsigned* data = NewArray<unsigned>(total_size);
  preamble_[PreparseDataConstants::kFunctionsSizeOffset] = function_size;
  MemCopy(data, preamble_, sizeof(preamble_));
  if (function_size > 0) {
    function_store_.WriteTo(Vector<unsigned>(
        data + PreparseDataConstants::kHeaderSize, function_size));
  }
  DCHECK(IsAligned(reinterpret_cast<intptr_t>(data), kPointerAlignment));
  ScriptData* result = new ScriptData(reinterpret_cast<byte*>(data),
                                      total_size * sizeof(unsigned));
  result->AcquireDataOwnership();
  return result;
}


}  // namespace internal
}  // namespace v8.
