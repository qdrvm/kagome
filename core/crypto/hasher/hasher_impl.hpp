/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_HASHER_HASHER_IMPL_HPP_
#define KAGOME_CORE_CRYPTO_HASHER_HASHER_IMPL_HPP_

#include "crypto/hasher.hpp"
#include "macro/nodiscard.hpp"

namespace kagome::hash {

  class HasherImpl : public Hasher {
    using Hash128 = kagome::common::Hash128;
    using Hash256 = kagome::common::Hash256;
    using Buffer = kagome::common::Buffer;

   public:
    ~HasherImpl() override = default;

    NODISCARD Hash128 twox_128(const Buffer &buffer) const override;

    NODISCARD Hash256 twox_256(const Buffer &buffer) const override;

    NODISCARD Hash256 blake2_256(const Buffer &buffer) const override;

    NODISCARD Hash256 sha2_256(const Buffer &buffer) const override;
  };

}  // namespace kagome::hash

#endif  // KAGOME_CORE_CRYPTO_HASHER_HASHER_IMPL_HPP_
