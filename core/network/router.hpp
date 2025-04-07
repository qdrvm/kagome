/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

namespace libp2p::protocol {
  class Ping;
}

namespace kagome::network {
  class BlockAnnounceProtocol;
  class CollationProtocol;
  class ValidationProtocol;
  class ReqCollationProtocol;
  class ReqPovProtocol;
  class FetchChunkProtocol;
  class FetchChunkProtocolObsolete;
  class FetchAvailableDataProtocol;
  class StatementFetchingProtocol;
  class PropagateTransactionsProtocol;
  class WarpProtocol;
  class StateProtocol;
  class SyncProtocol;
  class GrandpaProtocol;
  class SendDisputeProtocol;
  class BeefyProtocol;
  class FetchAttestedCandidateProtocol;
  class BeefyJustificationProtocol;
  class LightProtocol;
  using Ping = libp2p::protocol::Ping;
}  // namespace kagome::network

namespace kagome::network {

  /**
   * Router, which reads and delivers different network messages to the
   * observers, responsible for their processing
   */
  class Router {
   public:
    virtual ~Router() = default;

    virtual std::shared_ptr<BlockAnnounceProtocol> getBlockAnnounceProtocol()
        const = 0;
    virtual std::shared_ptr<CollationProtocol> getCollationProtocol() const = 0;
    virtual std::shared_ptr<ValidationProtocol> getValidationProtocol()
        const = 0;
    virtual std::shared_ptr<ReqCollationProtocol> getReqCollationProtocol()
        const = 0;
    virtual std::shared_ptr<ReqPovProtocol> getReqPovProtocol() const = 0;
    virtual std::shared_ptr<FetchChunkProtocol> getFetchChunkProtocol()
        const = 0;
    virtual std::shared_ptr<FetchChunkProtocolObsolete>
    getFetchChunkProtocolObsolete() const = 0;
    virtual std::shared_ptr<FetchAttestedCandidateProtocol>
    getFetchAttestedCandidateProtocol() const = 0;
    virtual std::shared_ptr<FetchAvailableDataProtocol>
    getFetchAvailableDataProtocol() const = 0;
    virtual std::shared_ptr<StatementFetchingProtocol>
    getFetchStatementProtocol() const = 0;
    virtual std::shared_ptr<PropagateTransactionsProtocol>
    getPropagateTransactionsProtocol() const = 0;
    virtual std::shared_ptr<WarpProtocol> getWarpProtocol() const = 0;
    virtual std::shared_ptr<StateProtocol> getStateProtocol() const = 0;
    virtual std::shared_ptr<SyncProtocol> getSyncProtocol() const = 0;
    virtual std::shared_ptr<GrandpaProtocol> getGrandpaProtocol() const = 0;
    virtual std::shared_ptr<SendDisputeProtocol> getSendDisputeProtocol()
        const = 0;
    virtual std::shared_ptr<BeefyProtocol> getBeefyProtocol() const = 0;
    virtual std::shared_ptr<libp2p::protocol::Ping> getPingProtocol() const = 0;
  };
}  // namespace kagome::network
