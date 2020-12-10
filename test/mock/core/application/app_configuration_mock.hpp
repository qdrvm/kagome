/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_APPCONFIGURATIONMOCK
#define KAGOME_APPLICATION_APPCONFIGURATIONMOCK

#include "application/app_configuration.hpp"

#include <gmock/gmock.h>

namespace kagome::application {

  class AppConfigurationMock : public AppConfiguration {
   public:
    MOCK_CONST_METHOD0(genesisPath, boost::filesystem::path());

    MOCK_CONST_METHOD1(chainPath,
                       boost::filesystem::path(std::string chain_id));

    MOCK_CONST_METHOD1(databasePath,
                       boost::filesystem::path(std::string chain_id));

    MOCK_CONST_METHOD1(keystorePath,
                       boost::filesystem::path(std::string chain_id));

    MOCK_CONST_METHOD0(p2pPort, uint16_t());

    MOCK_CONST_METHOD0(bootNodes,
                       const std::vector<libp2p::multi::Multiaddress> &());

    MOCK_CONST_METHOD0(rpcHttpEndpoint,
                       const boost::asio::ip::tcp::endpoint &());

    MOCK_CONST_METHOD0(rpcWsEndpoint,
                       const boost::asio::ip::tcp::endpoint &());

    MOCK_CONST_METHOD0(verbosity, spdlog::level::level_enum());

    MOCK_CONST_METHOD0(isOnlyFinalizing, bool());

    MOCK_CONST_METHOD0(isAlreadySynchronized, bool());

    MOCK_CONST_METHOD0(maxBlocksInResponse, uint32_t());

    MOCK_CONST_METHOD0(isUnixSlotsStrategy, bool());
  };

}  // namespace kagome::application

#endif  // KAGOME_APPLICATION_APPCONFIGURATIONMOCK
