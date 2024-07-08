/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "injector/application_injector.hpp"

#include <gtest/gtest.h>
#include <fstream>

#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "filesystem/common.hpp"
#include "mock/core/application/app_configuration_mock.hpp"
#include "network/impl/router_libp2p.hpp"
#include "testutil/storage/base_fs_test.hpp"
#include "utils/watchdog.hpp"

namespace fs = kagome::filesystem;
using testing::_;

namespace {
  /**
   * Write keys required by a validating node to \param keystore_dir .
   */
  void writeKeys(const fs::path &keystore_dir) {
    auto random_generator =
        std::make_shared<kagome::crypto::BoostRandomGenerator>();
    auto hasher = std::make_shared<kagome::crypto::HasherImpl>();
    auto sr25519_provider =
        std::make_shared<kagome::crypto::Sr25519ProviderImpl>();

    auto ed25519_provider =
        std::make_shared<kagome::crypto::Ed25519ProviderImpl>(hasher);

    {
      auto seed =
          kagome::crypto::Sr25519Seed::from(
              kagome::crypto::SecureCleanGuard{random_generator->randomBytes(
                  kagome::crypto::constants::sr25519::SEED_SIZE)})
              .value();
      auto babe = sr25519_provider->generateKeypair(seed, {}).value();
      auto babe_path =
          (keystore_dir / fmt::format("babe{}", babe.public_key.toHex()))
              .native();
      std::ofstream babe_file{babe_path};
      babe_file << kagome::common::hex_lower_0x(seed.unsafeBytes());
    }
    {
      auto seed =
          kagome::crypto::Ed25519Seed::from(
              kagome::crypto::SecureCleanGuard{random_generator->randomBytes(
                  kagome::crypto::Ed25519Seed::size())})
              .value();
      auto grandpa = ed25519_provider->generateKeypair(seed, {}).value();
      auto grandpa_path =
          (keystore_dir / fmt::format("gran{}", grandpa.public_key.toHex()))
              .native();
      std::ofstream grandpa_file{grandpa_path};
      auto hex = kagome::common::hex_lower(grandpa.secret_key.unsafeBytes());
      grandpa_file.write(hex.c_str(), hex.size());
    }
    {
      auto seed =
          kagome::crypto::Sr25519Seed::from(
              kagome::crypto::SecureCleanGuard{random_generator->randomBytes(
                  kagome::crypto::constants::sr25519::SEED_SIZE)})
              .value();
      auto libp2p = sr25519_provider->generateKeypair(seed, {}).value();
      auto libp2p_path =
          (keystore_dir / fmt::format("lp2p{}", libp2p.public_key.toHex()))
              .native();
      std::ofstream libp2p_file{libp2p_path};
      libp2p_file << kagome::common::hex_lower_0x(seed.unsafeBytes());
    }
  }

  /**
   * Initialize expectations on the \param config_mock
   */
  void initConfig(const fs::path &db_path,
                  kagome::application::AppConfigurationMock &config_mock) {
    static const auto chain_spec_path = fs::path(__FILE__)
                                            .parent_path()
                                            .parent_path()
                                            .parent_path()
                                            .parent_path()
                                      / "examples" / "polkadot"
                                      / "polkadot.json";
    EXPECT_CALL(config_mock, chainSpecPath())
        .WillRepeatedly(testing::Return(chain_spec_path));
    EXPECT_CALL(config_mock, databasePath(_))
        .WillRepeatedly(testing::Return(db_path));
    EXPECT_CALL(config_mock, keystorePath(_))
        .WillRepeatedly(testing::Return(db_path / "keys"));
    kagome::network::Roles roles;
    roles.flags.full = 1;
    roles.flags.authority = 1;
    EXPECT_CALL(config_mock, roles()).WillRepeatedly(testing::Return(roles));
    static auto key = std::make_optional(kagome::crypto::Ed25519Seed{});
    EXPECT_CALL(config_mock, nodeKey()).WillRepeatedly(testing::ReturnRef(key));
    EXPECT_CALL(config_mock, listenAddresses())
        .WillRepeatedly(
            testing::ReturnRefOfCopy<std::vector<libp2p::multi::Multiaddress>>(
                {}));
    EXPECT_CALL(config_mock, publicAddresses())
        .WillRepeatedly(
            testing::ReturnRefOfCopy<std::vector<libp2p::multi::Multiaddress>>(
                {}));
    EXPECT_CALL(config_mock, bootNodes())
        .WillRepeatedly(
            testing::ReturnRefOfCopy<std::vector<libp2p::multi::Multiaddress>>(
                {}));
    EXPECT_CALL(config_mock, rpcEndpoint())
        .WillRepeatedly(
            testing::ReturnRefOfCopy<boost::asio::ip::tcp::endpoint>({}));
    EXPECT_CALL(config_mock, openmetricsHttpEndpoint())
        .WillRepeatedly(
            testing::ReturnRefOfCopy<boost::asio::ip::tcp::endpoint>({}));
    EXPECT_CALL(config_mock, runtimeExecMethod())
        .WillRepeatedly(testing::Return(kagome::application::AppConfiguration::
                                            RuntimeExecutionMethod::Interpret));
    EXPECT_CALL(config_mock, parachainRuntimeInstanceCacheSize())
        .WillRepeatedly(testing::Return(100));
  }
}  // namespace

class KagomeInjectorTest : public test::BaseFS_Test {
 public:
  KagomeInjectorTest() : BaseFS_Test{db_path_} {}

  void SetUp() override {
    writeKeys(db_path_ / "keys");
    config_ = std::make_shared<kagome::application::AppConfigurationMock>();

    std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
    EXPECT_CALL(*config_, runtimeCacheDirPath())
        .WillRepeatedly(::testing::Return(temp_dir));

    initConfig(db_path_, *config_);
    injector_ = std::make_unique<kagome::injector::KagomeNodeInjector>(config_);
  }

  void TearDown() override {
    injector_->injectWatchdog()->stop();
    injector_.reset();
  }

 protected:
  inline static const auto db_path_ =
      fs::temp_directory_path() / fs::unique_path();

  std::shared_ptr<kagome::application::AppConfigurationMock> config_;
  std::unique_ptr<kagome::injector::KagomeNodeInjector> injector_;
};

#define TEST_KAGOME_INJECT(module) \
  ASSERT_NE(injector_->inject##module(), nullptr)

TEST_F(KagomeInjectorTest, Inject) {
  // Order as in KagomeApplicationImpl::run()
  TEST_KAGOME_INJECT(ChainSpec);
  TEST_KAGOME_INJECT(AppStateManager);
  TEST_KAGOME_INJECT(IoContext);
  TEST_KAGOME_INJECT(SystemClock);
  TEST_KAGOME_INJECT(Timeline);
  TEST_KAGOME_INJECT(OpenMetricsService);
  TEST_KAGOME_INJECT(Grandpa);
  TEST_KAGOME_INJECT(Router);
  TEST_KAGOME_INJECT(PeerManager);
  TEST_KAGOME_INJECT(RpcApiService);
  TEST_KAGOME_INJECT(StateObserver);
  TEST_KAGOME_INJECT(SyncObserver);
  TEST_KAGOME_INJECT(ParachainObserver);
  TEST_KAGOME_INJECT(MetricsWatcher);
  TEST_KAGOME_INJECT(TelemetryService);
  TEST_KAGOME_INJECT(ApprovalDistribution);
  TEST_KAGOME_INJECT(ParachainProcessor);
  TEST_KAGOME_INJECT(AddressPublisher);
}

TEST_F(KagomeInjectorTest, InjectProtocols) {
  auto router = injector_->injectRouter();
  ASSERT_NE(router, nullptr);

  EXPECT_NE(router->getBlockAnnounceProtocol(), nullptr);
  EXPECT_NE(router->getPropagateTransactionsProtocol(), nullptr);
  EXPECT_NE(router->getStateProtocol(), nullptr);
  EXPECT_NE(router->getSyncProtocol(), nullptr);
  EXPECT_NE(router->getGrandpaProtocol(), nullptr);
  EXPECT_NE(router->getCollationProtocol(), nullptr);
  EXPECT_NE(router->getValidationProtocol(), nullptr);
  EXPECT_NE(router->getReqCollationProtocol(), nullptr);
  EXPECT_NE(router->getReqPovProtocol(), nullptr);
  EXPECT_NE(router->getFetchChunkProtocol(), nullptr);
  EXPECT_NE(router->getFetchAvailableDataProtocol(), nullptr);
  EXPECT_NE(router->getFetchStatementProtocol(), nullptr);
  EXPECT_NE(router->getPingProtocol(), nullptr);
}
