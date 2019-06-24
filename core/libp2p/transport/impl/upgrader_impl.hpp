/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_UPGRADER_IMPL_HPP
#define KAGOME_UPGRADER_IMPL_HPP

#include <vector>

#include <gsl/span>
#include "libp2p/muxer/muxer_adaptor.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "libp2p/peer/protocol.hpp"
#include "libp2p/protocol_muxer/protocol_muxer.hpp"
#include "libp2p/security/security_adaptor.hpp"
#include "libp2p/transport/upgrader.hpp"

namespace libp2p::transport {
  class UpgraderImpl : public Upgrader {
    using RawSPtr = std::shared_ptr<connection::RawConnection>;
    using SecureSPtr = std::shared_ptr<connection::SecureConnection>;
    using CapableSPtr = std::shared_ptr<connection::CapableConnection>;

    using SecAdaptorSPtr = std::shared_ptr<security::SecurityAdaptor>;
    using MuxAdaptorSPtr = std::shared_ptr<muxer::MuxerAdaptor>;

   public:
    /**
     * Create an instance of upgrader
     * @param peer_id - id of this peer
     * @param protocol_muxer - protocol wrapper, allowing to negotiate about the
     * protocols with the other side
     * @param security_adaptors, which can be used to upgrade Raw connections to
     * the Secure ones
     * @param muxer_adaptors, which can be used to upgrade Secure connections to
     * the Muxed (Capable) ones
     */
    UpgraderImpl(peer::PeerId peer_id,
                 std::shared_ptr<protocol_muxer::ProtocolMuxer> protocol_muxer,
                 gsl::span<SecAdaptorSPtr> security_adaptors,
                 gsl::span<MuxAdaptorSPtr> muxer_adaptors);

    outcome::result<SecureSPtr> upgradeToSecure(RawSPtr conn) override;

    outcome::result<CapableSPtr> upgradeToMuxed(SecureSPtr conn) override;

    enum class Error { INTERNAL_ERROR = 1 };

   private:
    peer::PeerId peer_id_;

    std::shared_ptr<protocol_muxer::ProtocolMuxer> protocol_muxer_;

    std::vector<SecAdaptorSPtr> security_adaptors_;
    std::vector<peer::Protocol> security_protocols_;

    std::vector<MuxAdaptorSPtr> muxer_adaptors_;
    std::vector<peer::Protocol> muxer_protocols_;
  };
}  // namespace libp2p::transport

OUTCOME_HPP_DECLARE_ERROR(libp2p::transport, UpgraderImpl::Error)

#endif  // KAGOME_UPGRADER_IMPL_HPP
