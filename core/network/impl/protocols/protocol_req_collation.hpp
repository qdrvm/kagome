/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_REQCOLLATIONPROTOCOL
#define KAGOME_NETWORK_REQCOLLATIONPROTOCOL

#include "network/protocol_base.hpp"

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>

#include "application/app_configuration.hpp"
#include "application/chain_spec.hpp"
#include "log/logger.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/peer_manager.hpp"
#include "network/protocols/req_collation_protocol.hpp"
#include "network/types/collator_messages_vstaging.hpp"
#include "network/types/roles.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::blockchain {
  class GenesisBlockHash;
}

namespace kagome::network {

  template <typename RequestT, typename ResponseT>
  struct ReqCollationProtocolImpl;

  class ReqCollationProtocol final : public IReqCollationProtocol,
                                     NonCopyable,
                                     NonMovable {
   public:
    ReqCollationProtocol() = delete;
    ~ReqCollationProtocol() override = default;

    ReqCollationProtocol(libp2p::Host &host,
                         const application::ChainSpec &chain_spec,
                         const blockchain::GenesisBlockHash &genesis_hash,
                         std::shared_ptr<ReqCollationObserver> observer);

    const Protocol &protocolName() const override;

    bool start() override;

    void onIncomingStream(std::shared_ptr<Stream> stream) override;
    void newOutgoingStream(
        const PeerInfo &peer_info,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb)
        override;

    void request(const PeerId &peer_id,
                 CollationFetchingRequest request,
                 std::function<void(outcome::result<CollationFetchingResponse>)>
                     &&response_handler) override;

    void request(const PeerId &peer_id,
                 vstaging::CollationFetchingRequest request,
                 std::function<
                     void(outcome::result<vstaging::CollationFetchingResponse>)>
                     &&response_handler) override;

   private:
    std::shared_ptr<ReqCollationProtocolImpl<CollationFetchingRequest,
                                             CollationFetchingResponse>>
        v1_impl_;
    std::shared_ptr<
        ReqCollationProtocolImpl<vstaging::CollationFetchingRequest,
                                 vstaging::CollationFetchingResponse>>
        vstaging_impl_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_REQCOLLATIONPROTOCOL
