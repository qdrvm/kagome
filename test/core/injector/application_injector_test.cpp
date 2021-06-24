/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "injector/application_injector.hpp"

#include <gtest/gtest.h>

#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "mock/core/application/app_configuration_mock.hpp"
#include "testutil/prepare_loggers.hpp"

namespace fs = boost::filesystem;
using testing::_;

namespace {
  /**
   * Write keys required by a validating node to \param keystore_dir .
   */
  void writeKeys(const fs::path &keystore_dir) {
    auto random_generator =
        std::make_shared<kagome::crypto::BoostRandomGenerator>();
    auto sr25519_provider =
        std::make_shared<kagome::crypto::Sr25519ProviderImpl>(random_generator);

    auto ed25519_provider =
        std::make_shared<kagome::crypto::Ed25519ProviderImpl>(random_generator);

    {
      auto seed = kagome::crypto::Sr25519Seed::fromSpan(
                      random_generator->randomBytes(
                          kagome::crypto::constants::sr25519::SEED_SIZE))
                      .value();
      auto babe = sr25519_provider->generateKeypair(seed);
      auto babe_path =
          (keystore_dir / fmt::format("babe{}", babe.public_key.toHex()))
              .native();
      std::ofstream babe_file{babe_path};
      babe_file << seed.toHex();
    }
    {
      auto grandpa = ed25519_provider->generateKeypair();
      auto grandpa_path =
          (keystore_dir / fmt::format("gran{}", grandpa.public_key.toHex()))
              .native();
      std::ofstream grandpa_file{grandpa_path};
      grandpa_file << grandpa.secret_key.toHex();
    }
    {
      auto seed = kagome::crypto::Sr25519Seed::fromSpan(
                      random_generator->randomBytes(
                          kagome::crypto::constants::sr25519::SEED_SIZE))
                      .value();
      auto libp2p = sr25519_provider->generateKeypair(seed);
      auto libp2p_path =
          (keystore_dir / fmt::format("lp2p{}", libp2p.public_key.toHex()))
              .native();
      std::ofstream libp2p_file{libp2p_path};
      libp2p_file << seed.toHex();
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
    EXPECT_CALL(config_mock, isUnixSlotsStrategy())
        .WillRepeatedly(testing::Return(true));
    static auto key = boost::make_optional(kagome::crypto::Ed25519PrivateKey{});
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
    EXPECT_CALL(config_mock, rpcHttpEndpoint())
        .WillRepeatedly(
            testing::ReturnRefOfCopy<boost::asio::ip::tcp::endpoint>({}));
    EXPECT_CALL(config_mock, rpcWsEndpoint())
        .WillRepeatedly(
            testing::ReturnRefOfCopy<boost::asio::ip::tcp::endpoint>({}));
    EXPECT_CALL(config_mock, openmetricsHttpEndpoint())
        .WillRepeatedly(
            testing::ReturnRefOfCopy<boost::asio::ip::tcp::endpoint>({}));
  }
}  // namespace

class KagomeInjectorTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
    fs::create_directories(db_path_);
    fs::create_directories(db_path_ / "keys");
    writeKeys(db_path_ / "keys");
  }

  void SetUp() override {
    config_ = std::make_shared<kagome::application::AppConfigurationMock>();
    initConfig(db_path_, *config_);
    injector_ =
        std::make_unique<kagome::injector::KagomeNodeInjector>(*config_);
  }

  static void TearDownTestCase() {
    fs::remove_all(db_path_);
  }

 protected:
  static inline const auto db_path_ =
      fs::temp_directory_path() / fs::unique_path();

  std::shared_ptr<kagome::application::AppConfigurationMock> config_;
  std::unique_ptr<kagome::injector::KagomeNodeInjector> injector_;
};

#define TEST_KAGOME_INJECT(module)                   \
  TEST_F(KagomeInjectorTest, Inject##module) {       \
    ASSERT_NE(injector_->inject##module(), nullptr); \
  }

TEST_KAGOME_INJECT(ChainSpec)
TEST_KAGOME_INJECT(AppStateManager)
TEST_KAGOME_INJECT(IoContext)
TEST_KAGOME_INJECT(OpenMetricsService)
TEST_KAGOME_INJECT(Router)
TEST_KAGOME_INJECT(PeerManager)
TEST_KAGOME_INJECT(RpcApiService)
TEST_KAGOME_INJECT(SystemClock)
TEST_KAGOME_INJECT(SyncObserver)
TEST_KAGOME_INJECT(Babe)
TEST_KAGOME_INJECT(Grandpa)
