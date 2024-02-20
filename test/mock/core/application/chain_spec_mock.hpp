/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/chain_spec.hpp"

#include <gmock/gmock.h>

namespace kagome::application {

  class ChainSpecMock : public ChainSpec {
   public:
    MOCK_METHOD(const std::string &, name, (), (const, override));

    MOCK_METHOD(const std::string &, id, (), (const, override));

    MOCK_METHOD(const std::string &, chainType, (), (const, override));

    MOCK_METHOD(const std::vector<libp2p::multi::Multiaddress> &,
                bootNodes,
                (),
                (const, override));

    MOCK_METHOD((const std::vector<std::pair<std::string, size_t>> &),
                telemetryEndpoints,
                (),
                (const));

    MOCK_METHOD(const std::string &, protocolId, (), (const, override));

    MOCK_METHOD((const std::map<std::string, std::string> &),
                properties,
                (),
                (const));

    MOCK_METHOD(std::optional<std::reference_wrapper<const std::string>>,
                getProperty,
                (const std::string &property),
                (const, override));

    MOCK_METHOD(const std::set<primitives::BlockHash> &,
                forkBlocks,
                (),
                (const, override));

    MOCK_METHOD(const std::set<primitives::BlockHash> &,
                badBlocks,
                (),
                (const, override));

    MOCK_METHOD(std::optional<std::string>,
                consensusEngine,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<const primitives::CodeSubstituteBlockIds>,
                codeSubstitutes,
                (),
                (const, override));

    MOCK_METHOD(const GenesisRawData &,
                getGenesisTopSection,
                (),
                (const, override));

    MOCK_METHOD(const ChildrenDefaultRawData &,
                getGenesisChildrenDefaultSection,
                (),
                (const, override));

    MOCK_METHOD(outcome::result<common::Buffer>,
                fetchCodeSubstituteByBlockInfo,
                (const primitives::BlockInfo &block_info),
                (const, override));

    MOCK_METHOD(primitives::BlockNumber, beefyMinDelta, (), (const, override));
  };

}  // namespace kagome::application
