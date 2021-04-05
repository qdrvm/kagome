/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_SYNCPROTOCOL
#define KAGOME_NETWORK_SYNCPROTOCOL

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>

#include "application/chain_spec.hpp"
#include "blockchain/block_storage.hpp"
#include "blockchain/block_tree.hpp"
#include "crypto/hasher.hpp"
#include "log/logger.hpp"
#include "network/babe_observer.hpp"
#include "network/peer_manager.hpp"
#include "network/types/block_announce.hpp"
#include "network/types/status.hpp"
#include "outcome/outcome.hpp"

namespace kagome::network {

  using Stream = libp2p::connection::Stream;
  using Protocol = libp2p::peer::Protocol;
  using PeerId = libp2p::peer::PeerId;
  using PeerInfo = libp2p::peer::PeerInfo;

  class SyncProtocol final
      : public std::enable_shared_from_this<SyncProtocol> {
   public:
    enum class Error { CAN_NOT_CREATE_STATUS = 1, GONE };

    SyncProtocol() = delete;
    SyncProtocol(SyncProtocol &&) noexcept = delete;
    SyncProtocol(const SyncProtocol &) = delete;
    virtual ~SyncProtocol() = default;
    SyncProtocol &operator=(SyncProtocol &&) noexcept =
    delete;
    SyncProtocol &operator=(SyncProtocol const &) = delete;

    SyncProtocol(
        libp2p::Host &host,
        const application::ChainSpec &chain_spec,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<blockchain::BlockStorage> storage,
        //                          std::shared_ptr<PeerManager> peer_manager,
        std::shared_ptr<BabeObserver> babe_observer,
        std::shared_ptr<crypto::Hasher> hasher);
    bool start();
    bool stop();

    void onIncomingStream(std::shared_ptr<Stream> stream);
    void newOutgoingStream(
        const PeerInfo &peer_info,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb);

   private:
    outcome::result<Status> createStatus() const;

    enum class Direction { INCOMING, OUTGOING };
    void readStatus(
        std::shared_ptr<Stream> stream,
        Direction direction,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb);

    void writeStatus(
        std::shared_ptr<Stream> stream,
        Direction direction,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb);

    void readAnnounce(std::shared_ptr<Stream> stream);
    void writeAnnounce(std::shared_ptr<Stream> stream,
                       const BlockAnnounce &block_announce);

    libp2p::Host &host_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<blockchain::BlockStorage> storage_;
    //    std::shared_ptr<PeerManager> peer_manager_;
    std::shared_ptr<BabeObserver> babe_observer_;
    std::shared_ptr<crypto::Hasher> hasher_;
    const libp2p::peer::Protocol protocol_;
    log::Logger log_ = log::createLogger("SyncProtocol");
  };

}  // namespace kagome::network

OUTCOME_HPP_DECLARE_ERROR(kagome::network, SyncProtocol::Error);

#endif  // KAGOME_NETWORK_SYNCPROTOCOL
