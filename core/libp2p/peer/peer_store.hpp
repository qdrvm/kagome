/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_STORE_HPP
#define KAGOME_PEER_STORE_HPP

#include "libp2p/store/keyvalue.hpp"
#include "libp2p/peer/address_repository.hpp"
#include "libp2p/peer/key_repository.hpp"
#include "libp2p/peer/peer_info.hpp"

namespace libp2p::peer {

  class PeerStore {
    using StringKV = store::StringKV;

   public:
    virtual ~PeerStore() = default;

    virtual PeerInfo getPeerInfo(PeerId id) = 0;

    virtual std::shared_ptr<AddressRepository> getAddressStore() noexcept = 0;

    virtual std::shared_ptr<KeyRepository> getKeyRepository() noexcept = 0;

    virtual std::shared_ptr<StringKV> getPeerMetadataStore() noexcept = 0;
  };

}  // namespace libp2p::peer

#endif  // KAGOME_PEER_STORE_HPP
