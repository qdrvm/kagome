#pragma once

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "crypto/keccak/keccak.h"

namespace kagome::crypto {
  inline common::Hash256 keccak(common::BufferView buf) {
    common::Hash256 out;
    sha3_HashBuffer(256,
                    SHA3_FLAGS::SHA3_FLAGS_KECCAK,
                    buf.data(),
                    buf.size(),
                    out.data(),
                    32);
    return out;
  }
}  // namespace kagome::crypto
