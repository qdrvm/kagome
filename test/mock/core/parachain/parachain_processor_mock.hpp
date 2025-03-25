/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "parachain/validator/i_parachain_processor.hpp"

namespace kagome::parachain {

  class ParachainProcessorMock : public ParachainProcessor {
   public:
    ~ParachainProcessorMock() override = default;

    MOCK_METHOD(outcome::result<void>, canProcessParachains, (), (const, override));
    MOCK_METHOD(void, onValidationProtocolMsg, (const libp2p::peer::PeerId &, const network::VersionedValidatorProtocolMessage &), (override));
    MOCK_METHOD(void, handle_advertisement, ((const RelayHash &), (const libp2p::peer::PeerId &), (std::optional<std::pair<CandidateHash, Hash>> &&)), (override));
    MOCK_METHOD(void, onIncomingCollator, (const libp2p::peer::PeerId &, network::CollatorPublicKey, network::ParachainId), (override));
    MOCK_METHOD(void, handleStatement, (const primitives::BlockHash &, const SignedFullStatementWithPVD &), (override));
  };

}  // namespace kagome::parachain 