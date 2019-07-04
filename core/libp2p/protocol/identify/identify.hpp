/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_IDENTIFY_IMPL_HPP
#define KAGOME_IDENTIFY_IMPL_HPP

#include <memory>
#include <string_view>
#include <vector>

#include "common/logger.hpp"
#include "libp2p/connection/capable_connection.hpp"
#include "libp2p/connection/stream.hpp"
#include "libp2p/crypto/key_marshaller.hpp"
#include "libp2p/event/bus.hpp"
#include "libp2p/host_impl.hpp"
#include "libp2p/peer/identity_manager.hpp"
#include "libp2p/protocol/identify/observed_addresses.hpp"

namespace libp2p::multi {
  class Multiaddress;
}

namespace libp2p::protocol {
  /**
   * Implementation of an Identify protocol, which is a way to say
   * "hello" to the other peer, sending our listen addresses, ID, etc
   * Read more: https://github.com/libp2p/specs/tree/master/identify
   */
  class Identify : public std::enable_shared_from_this<Identify> {
    using HostSPtr = std::shared_ptr<HostImpl>;
    using StreamSPtr = std::shared_ptr<connection::Stream>;
    using BufSPtr = std::shared_ptr<kagome::common::Buffer>;

   public:
    /**
     * Create an Identify instance; it will immediately start watching
     * connection events and react to them
     * @param host - this Libp2p instance
     * @param event_bus - bus, over which the events arrive
     * @param identity_manager - this peer's identity manager
     * @param key_marshaller - marshaller of private & public keys
     * @param log to write information to
     */
    Identify(
        HostSPtr host, event::Bus &event_bus,
        peer::IdentityManager &identity_manager,
        std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller,
        kagome::common::Logger log = kagome::common::createLogger("Identify"));

    /**
     * Get addresses other peers reported we have dialed from
     * @return set of addresses
     */
    std::vector<multi::Multiaddress> getAllObservedAddresses() const;

    /**
     * Get addresses other peers reported we have dialed from, when they
     * provided a (\param address)
     * @param address, for which to retrieve observed addresses
     * @return set of addresses
     */
    std::vector<multi::Multiaddress> getObservedAddressesFor(
        const multi::Multiaddress &address) const;

   private:
    /**
     * Handler for new connections, established by or with our host
     * @param conn - new connection
     */
    void onNewConnection(
        const std::weak_ptr<connection::CapableConnection> &conn);

    /**
     * Handler for the case, when we are being identified by the other peer; we
     * should respond with an Identify message and close the stream
     * @param stream to be identified over
     */
    void sendIdentify(StreamSPtr stream);

    /**
     * Called, when an identify message is written to the stream
     * @param written_bytes - how much bytes were written
     * @param stream with the other side
     */
    void identifySent(outcome::result<size_t> written_bytes,
                      const StreamSPtr &stream);

    /**
     * Handler for the case, when we want to identify the other side
     * @param stream to be identified over
     */
    void receiveIdentify(StreamSPtr stream);

    /**
     * Called, when an identify message is received from the other peer
     * @param read_bytes - how much bytes were read
     * @param stream, over which it was received
     * @param buffer - bytes, which were read
     */
    void identifyReceived(outcome::result<size_t> read_bytes,
                          const StreamSPtr &stream, const BufSPtr &buffer);

    /**
     * Process a received public key of the other peer
     * @param stream, over which the key was received
     * @param pubkey_str - marshalled public key; can be empty, if there was no
     * public key in the message
     * @return peer id, which was derived from the provided public key (if it
     * can be derived)
     */
    std::optional<peer::PeerId> consumePublicKey(const StreamSPtr &stream,
                                                 std::string_view pubkey_str);

    /**
     * Process received address, which the other peer used to connect to us
     * @param address - observed address string
     * @param peer_id - ID of that peer
     * @param stream, over which the message came
     */
    void consumeObservedAddresses(const std::string &address_str,
                                  const peer::PeerId &peer_id,
                                  const StreamSPtr &stream);

    /**
     * Process received addresses, which the other peer listens to
     * @param addresses_strings - stringified listen addresses
     * @param peer_id - ID of that peer
     */
    void consumeListenAddresses(gsl::span<const std::string> addresses_strings,
                                const peer::PeerId &peer_id);

    HostSPtr host_;
    event::Bus &bus_;
    peer::IdentityManager &identity_manager_;

    std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller_;

    ObservedAddresses observed_addresses_;

    kagome::common::Logger log_;
  };
}  // namespace libp2p::protocol

#endif  // KAGOME_IDENTIFY_IMPL_HPP
