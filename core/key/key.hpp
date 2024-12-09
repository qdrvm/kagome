/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include <libp2p/crypto/key_marshaller.hpp>
#include "crypto/ed25519_provider.hpp"

namespace kagome::key {

  class Key {
   public:
    Key(std::shared_ptr<crypto::Ed25519Provider> ed_crypto_provider,
        std::shared_ptr<libp2p::crypto::marshaller::KeyMarshaller>
            key_marshaller);

    outcome::result<void> run();

   private:
    std::shared_ptr<crypto::Ed25519Provider> ed_crypto_provider_;
    std::shared_ptr<libp2p::crypto::marshaller::KeyMarshaller> key_marshaller_;
  };

}  // namespace kagome::key
