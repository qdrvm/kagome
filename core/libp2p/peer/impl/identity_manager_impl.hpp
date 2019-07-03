/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_IDENTITY_MANAGER_IMPL_HPP
#define KAGOME_IDENTITY_MANAGER_IMPL_HPP

#include "libp2p/crypto/key_generator.hpp"
#include "libp2p/peer/identity_manager.hpp"
#include "libp2p/peer/key_repository.hpp"

namespace libp2p::peer {
  class IdentityManagerImpl : public IdentityManager {
   public:
    ~IdentityManagerImpl() override = default;

    explicit IdentityManagerImpl(crypto::KeyPair keyPair);

    const peer::PeerId &getId() const noexcept override;;

    const crypto::KeyPair &getKeyPair() const noexcept override;;

   private:
    std::unique_ptr<peer::PeerId> id_;
    std::unique_ptr<crypto::KeyPair> keyPair_;
  };

}  // namespace libp2p::peer

#endif  // KAGOME_IDENTITY_MANAGER_IMPL_HPP
