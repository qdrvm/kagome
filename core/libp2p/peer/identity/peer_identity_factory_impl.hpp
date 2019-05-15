/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_IDENTITY_FACTORY_IMPL_HPP
#define KAGOME_PEER_IDENTITY_FACTORY_IMPL_HPP

#include <memory>

#include "libp2p/multi/multibase_codec.hpp"
#include "libp2p/peer/identity/peer_identity_factory.hpp"

namespace libp2p::peer {
  class PeerIdentityFactoryImpl : public PeerIdentityFactory {
   public:
    explicit PeerIdentityFactoryImpl(
        std::shared_ptr<multi::MultibaseCodec> codec);

    ~PeerIdentityFactoryImpl() override = default;

    enum class FactoryError { ID_EXPECTED = 1, NO_ADDRESSES, SHA256_EXPECTED };

    FactoryResult create(std::string_view identity) const override;

    FactoryResult create(const PeerInfo &peer_info) const override;

    FactoryResult create(const PeerId &peer_id,
                         const multi::Multiaddress &address) const override;

   private:
    std::shared_ptr<multi::MultibaseCodec> codec_;
  };
}  // namespace libp2p::peer

OUTCOME_HPP_DECLARE_ERROR(libp2p::peer, PeerIdentityFactoryImpl::FactoryError)

#endif  // KAGOME_PEER_IDENTITY_FACTORY_IMPL_HPP
