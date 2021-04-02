/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "injector/application_injector.hpp"

#include <gtest/gtest.h>

#include "application/impl/app_configuration_impl.hpp"
#include "testutil/prepare_loggers.hpp"

static auto initConfig(kagome::application::AppConfigurationImpl &config) {
  static boost::filesystem::path tmp_dir =
      boost::filesystem::temp_directory_path()
      / boost::filesystem::unique_path();
  static auto genesis_path = boost::filesystem::path(__FILE__).parent_path().parent_path()
                             / "application" / "genesis.json";
  static auto base_path = tmp_dir / "base_path";

  boost::filesystem::create_directories(base_path);

  const char *argv[5]{
      "", "-g", genesis_path.native().data(), "-d", base_path.native().data()};
  ASSERT_TRUE(config.initialize_from_args(
      kagome::application::AppConfiguration::LoadScheme::kValidating,
      5,
      const_cast<char **>(argv)));
}

class SyncingInjectorTest : public testing::Test {
 public:
  void SetUp() override {
    testutil::prepareLoggers();
    config_ = std::make_shared<kagome::application::AppConfigurationImpl>(
        kagome::log::createLogger("SyncingInjectorTest"));
    initConfig(*config_);
    injector_ =
        std::make_unique<kagome::injector::SyncingNodeInjector>(*config_);
  }

 protected:
  std::shared_ptr<kagome::application::AppConfigurationImpl> config_;

  std::unique_ptr<kagome::injector::SyncingNodeInjector> injector_;
};

class ValidatingInjectorTest : public testing::Test {
 public:
  void SetUp() override {
    testutil::prepareLoggers();
    config_ = std::make_shared<kagome::application::AppConfigurationImpl>(
        kagome::log::createLogger("ValidatingInjectorTest"));

    initConfig(*config_);
    injector_ =
        std::make_unique<kagome::injector::ValidatingNodeInjector>(*config_);
  }

 protected:
  std::shared_ptr<kagome::application::AppConfigurationImpl> config_;
  std::unique_ptr<kagome::injector::ValidatingNodeInjector> injector_;
};

#define TEST_SYNCING_INJECT(module)                  \
  TEST_F(SyncingInjectorTest, Inject##module) {      \
    ASSERT_NE(injector_->inject##module(), nullptr); \
  }

#define TEST_VALIDATING_INJECT(module)               \
  TEST_F(ValidatingInjectorTest, Inject##module) {   \
    ASSERT_NE(injector_->inject##module(), nullptr); \
  }

TEST_SYNCING_INJECT(ChainSpec)
TEST_SYNCING_INJECT(AppStateManager)
TEST_SYNCING_INJECT(Router)
TEST_SYNCING_INJECT(PeerManager)
TEST_SYNCING_INJECT(RpcApiService)

TEST_VALIDATING_INJECT(ChainSpec)
TEST_VALIDATING_INJECT(AppStateManager)
TEST_VALIDATING_INJECT(Router)
TEST_VALIDATING_INJECT(PeerManager)
TEST_VALIDATING_INJECT(RpcApiService)

TEST_VALIDATING_INJECT(Grandpa)
TEST_VALIDATING_INJECT(Babe)
TEST_VALIDATING_INJECT(SystemClock)
TEST_VALIDATING_INJECT(LoggingSystem)
