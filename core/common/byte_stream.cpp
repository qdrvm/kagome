/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/byte_stream.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::common, ByteStream::AdvanceErrc, e) {
  using kagome::common::ByteStream;
  switch (e) {
    case ByteStream::AdvanceErrc::OUT_OF_BOUNDARIES:
      return "Advance cannot move pointer outside of boundaries";
  }

  return "Unknown error";
}
