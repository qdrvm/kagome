/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ADDRESS_REPOSITORY_HPP
#define KAGOME_ADDRESS_REPOSITORY_HPP

#include <chrono>
#include <list>

#include <boost/signals2.hpp>
#include <gsl/span>
#include "libp2p/basic/garbage_collectable.hpp"
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/peer/peer_id.hpp"

namespace libp2p::peer {

  namespace ttl {

    constexpr const auto kPermanent = std::chrono::milliseconds::max();

    constexpr const auto kHour = std::chrono::hours(1);

  }  // namespace ttl

  /**
   * @brief Address Repository is a storage of multiaddresses of observed peers.
   */
  class AddressRepository : public basic::GarbageCollectable {
   protected:
    using Milliseconds = std::chrono::milliseconds;

   public:
    using AddressCallback = void(const PeerId &, const multi::Multiaddress &);

    ~AddressRepository() override = default;

    virtual outcome::result<void> addAddresses(
        const PeerId &p, gsl::span<const multi::Multiaddress> ma,
        Milliseconds ttl) = 0;

    /**
     * @brief Update existing addresses with new {@param ttl} or insert new
     * addresses with new {@param ttl}
     * @param p peer
     * @param ma set of addresses
     * @param ttl ttl
     * @return error code if no peer found
     */
    virtual outcome::result<void> upsertAddresses(
        const PeerId &p, gsl::span<const multi::Multiaddress> ma,
        Milliseconds ttl) = 0;

    /**
     * @brief Get all addresses associated with this Peer {@param p}. May
     * contain duplicates.
     * @param p peer
     * @return array of addresses
     */
    virtual outcome::result<std::list<multi::Multiaddress>> getAddresses(
        const PeerId &p) const = 0;

    /**
     * @brief Clear all addresses of given Peer {@param p}. Does not evict peer
     * from the list of known peers up to the next garbage collection.
     * @param p peer
     *
     * @note triggers #onAddressRemoved for every removed address
     */
    virtual void clear(const PeerId &p) = 0;

    boost::signals2::connection onAddressAdded(
        const std::function<AddressCallback> &cb);

    boost::signals2::connection onAddressRemoved(
        const std::function<AddressCallback> &cb);

   protected:
    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    boost::signals2::signal<AddressCallback> signal_added_;
    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    boost::signals2::signal<AddressCallback> signal_removed_;
  };  // namespace libp2p::peer

}  // namespace libp2p::peer

#endif  // KAGOME_ADDRESS_REPOSITORY_HPP
