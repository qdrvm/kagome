/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_CHAINSPECMOCK
#define KAGOME_APPLICATION_CHAINSPECMOCK

#include "application/chain_spec.hpp"

#include <gmock/gmock.h>

namespace kagome::application {

  class ChainSpecMock : public ChainSpec {
   public:
    MOCK_CONST_METHOD0(name, const std::string &());

    MOCK_CONST_METHOD0(id, const std::string &());

    MOCK_CONST_METHOD0(chainType, const std::string &());

    MOCK_CONST_METHOD0(bootNodes,
                       const std::vector<libp2p::multi::Multiaddress> &());

    MOCK_CONST_METHOD0(telemetryEndpoints,
                       const std::vector<std::pair<std::string, size_t>> &());

    MOCK_CONST_METHOD0(protocolId, const std::string &());

    MOCK_CONST_METHOD0(properties,
                       const std::map<std::string, std::string> &());

    MOCK_CONST_METHOD1(
        getProperty,
        boost::optional<std::reference_wrapper<const std::string>>(
            const std::string &property));

    MOCK_CONST_METHOD0(forkBlocks, const std::set<primitives::BlockHash> &());

    MOCK_CONST_METHOD0(badBlocks, const std::set<primitives::BlockHash> &());

    MOCK_CONST_METHOD0(consensusEngine, boost::optional<std::string>());

    MOCK_CONST_METHOD0(codeSubstitutes, const primitives::CodeSubstitutes &());

    MOCK_CONST_METHOD0(getGenesis, GenesisRawData());
  };

}  // namespace kagome::application

#endif  // KAGOME_APPLICATION_CHAINSPECMOCK
