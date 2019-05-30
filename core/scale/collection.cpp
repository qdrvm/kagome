/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/collection.hpp"
#include "scale/scale_encoder_stream.hpp"

namespace kagome::scale::collection {
  outcome::result<std::string> decodeString(common::ByteStream &stream) {
    OUTCOME_TRY(s, collection::decodeCollection<unsigned char>(stream));
    return std::string(s.begin(), s.end());
  }
}  // namespace kagome::scale::collection
