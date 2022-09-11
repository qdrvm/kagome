/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_SR25519_SIGNED_HPP
#define KAGOME_CORE_CRYPTO_SR25519_SIGNED_HPP

#include <type_traits>

#include "common/blob.hpp"
#include "crypto/sr25519_types.hpp"
#include "scale/tie.hpp"

namespace kagome::crypto::sr25519 {

  using Signature = crypto::Sr25519Signature;
  using SignerIndex = uint32_t;

  template <typename D>
  struct Sr25519Signed {
    using Type = std::decay_t<D>;
    SCALE_TIE(3);

    Type payload;
    SignerIndex ix;
    Signature signature;
  };

}  // namespace kagome::crypto::sr25519

#endif  // KAGOME_CORE_CRYPTO_SR25519_SIGNED_HPP
