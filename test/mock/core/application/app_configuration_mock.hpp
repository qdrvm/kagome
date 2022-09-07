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
    MOCK_METHOD(network::Roles, roles, (), (const, override));

    MOCK_METHOD(boost::filesystem::path, chainSpecPath, (), (const, override));

    MOCK_METHOD(boost::filesystem::path,
                runtimeCacheDirPath,
                (),
                (const, override));

    MOCK_METHOD(boost::filesystem::path,
                runtimeCachePath,
                (std::string runtime_hash),
                (const, override));

    MOCK_METHOD(boost::filesystem::path,
                chainPath,
                (std::string chain_id),
                (const, override));

    MOCK_METHOD(boost::filesystem::path,
                databasePath,
                (std::string chain_id),
                (const, override));

    MOCK_METHOD(boost::filesystem::path,
                keystorePath,
                (std::string chain_id),
                (const, override));

    MOCK_METHOD(const std::optional<crypto::Ed25519PrivateKey> &,
                nodeKey,
                (),
                (const, override));

    MOCK_METHOD(const std::optional<std::string> &,
                nodeKeyFile,
                (),
                (const, override));

    MOCK_METHOD(bool, shouldSaveNodeKey, (), (const, override));

    MOCK_METHOD(const std::vector<libp2p::multi::Multiaddress> &,
                listenAddresses,
                (),
                (const, override));

    MOCK_METHOD(const std::vector<libp2p::multi::Multiaddress> &,
                publicAddresses,
                (),
                (const, override));

    MOCK_METHOD(const std::vector<libp2p::multi::Multiaddress> &,
                bootNodes,
                (),
                (const, override));

    MOCK_METHOD(uint16_t, p2pPort, (), (const, override));

    MOCK_METHOD(const boost::asio::ip::tcp::endpoint &,
                rpcHttpEndpoint,
                (),
                (const, override));

    MOCK_METHOD(const boost::asio::ip::tcp::endpoint &,
                rpcWsEndpoint,
                (),
                (const, override));

    MOCK_METHOD(const boost::asio::ip::tcp::endpoint &,
                openmetricsHttpEndpoint,
                (),
                (const, override));

    MOCK_METHOD(uint32_t, maxWsConnections, (), (const, override));

    MOCK_METHOD(std::chrono::seconds,
                getRandomWalkInterval,
                (),
                (const, override));

    MOCK_METHOD(const std::vector<std::string> &, log, (), (const, override));

    MOCK_METHOD(uint32_t, maxBlocksInResponse, (), (const, override));

    MOCK_METHOD(const network::PeeringConfig &,
                peeringConfig,
                (),
                (const, override));

    MOCK_METHOD(bool, isRunInDevMode, (), (const, override));

    MOCK_METHOD(const std::string &, nodeName, (), (const, override));

    MOCK_METHOD(const std::string &, nodeVersion, (), (const, override));

    MOCK_METHOD(const std::vector<telemetry::TelemetryEndpoint> &,
                telemetryEndpoints,
                (),
                (const, override));

    MOCK_METHOD(AppConfiguration::SyncMethod, syncMethod, (), (const, override));

    MOCK_METHOD(AppConfiguration::RuntimeExecutionMethod,
                runtimeExecMethod,
                (),
                (const, override));

    MOCK_METHOD(bool, useWavmCache, (), (const, override));

    MOCK_METHOD(bool, purgeWavmCache, (), (const, override));

    MOCK_METHOD(AppConfiguration::OffchainWorkerMode,
                offchainWorkerMode,
                (),
                (const, override));

    MOCK_METHOD(bool, isOffchainIndexingEnabled, (), (const, override));

    MOCK_METHOD(bool, subcommandChainInfo, (), (const, override));

    MOCK_METHOD(std::optional<primitives::BlockId>,
                recoverState,
                (),
                (const, override));

    MOCK_METHOD(uint32_t, outPeers, (), (const, override));

    MOCK_METHOD(uint32_t, inPeers, (), (const, override));

    MOCK_METHOD(uint32_t, inPeersLight, (), (const, override));

    MOCK_METHOD(int32_t, luckyPeers, (), (const, override));

    MOCK_METHOD(bool, isTelemetryEnabled, (), (const, override));

    MOCK_METHOD(StorageBackend, storageBackend, (), (const, override));
  };

}  // namespace kagome::application

#endif  // KAGOME_APPLICATION_APPCONFIGURATIONMOCK
