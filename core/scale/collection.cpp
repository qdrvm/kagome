/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/collection.hpp"

namespace kagome::scale::collection {
  outcome::result<void> encodeBuffer(const common::Buffer &buf,
                                     common::Buffer &out) {
    OUTCOME_TRY(compact::encodeInteger(buf.size(), out));
    out.putBuffer(buf);
    return outcome::success();
  }

  outcome::result<void> encodeString(std::string_view string,
                                     common::Buffer &out) {
    OUTCOME_TRY(compact::encodeInteger(string.length(), out));
    out.put(std::string_view(string));
    return outcome::success();
  }
  outcome::result<std::string> decodeString(common::ByteStream &stream) {
    OUTCOME_TRY(s, collection::decodeCollection<unsigned char>(stream));
    return std::string(s.begin(), s.end());
  }
}  // namespace kagome::scale::collection
