/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_IDENTITY_MANAGER_MOCK_HPP
#define KAGOME_IDENTITY_MANAGER_MOCK_HPP

#include <gmock/gmock.h>
#include "libp2p/peer/identity_manager.hpp"

namespace libp2p::peer {
  class IdentityManagerMock : public IdentityManager {
    const peer::PeerId &getId() const noexcept override {
      return getId_hack();
    }
    MOCK_CONST_METHOD0(getId_hack, const peer::PeerId &());

    const crypto::KeyPair &getKeyPair() const noexcept override {
      return getKeyPair_hack();
    }
    MOCK_CONST_METHOD0(getKeyPair_hack, const crypto::KeyPair &());
  };
}  // namespace libp2p::peer

#endif  // KAGOME_IDENTITY_MANAGER_MOCK_HPP
