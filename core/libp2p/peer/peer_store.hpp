/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_STORE_HPP
#define KAGOME_PEER_STORE_HPP

#include "libp2p/peer/address_repository.hpp"
#include "libp2p/peer/key_repository.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/store/keyvalue.hpp"

namespace libp2p::peer {

  /**
   * @brief A component that is used to store information about other peers -
   * cryptographic material, addresses and metadata.
   */
  class PeerStore {
    using StringKV = store::StringKV;

   public:
    virtual ~PeerStore() = default;

    /**
     * @brief Get storage of multiaddresses.
     * @return pointer to address repository
     */
    virtual std::shared_ptr<AddressRepository> getAddressStore() noexcept = 0;

    /**
     * @brief Get storage of cryptographic material.
     * @return pointer to key repository
     */
    virtual std::shared_ptr<KeyRepository> getKeyRepository() noexcept = 0;

    // TODO(@warchant): implement this when leveldb interfaces are merged
    //    virtual std::shared_ptr<StringKV> getPeerMetadataStore() noexcept = 0;
  };

}  // namespace libp2p::peer

#endif  // KAGOME_PEER_STORE_HPP
