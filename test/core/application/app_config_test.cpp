/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <fstream>

#include "application/impl/app_configuration_impl.hpp"
#include "log/logger.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::application::AppConfiguration;
using kagome::application::AppConfigurationImpl;

class AppConfigurationTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  boost::filesystem::path tmp_dir = boost::filesystem::temp_directory_path()
                                    / boost::filesystem::unique_path();
  std::string config_path = (tmp_dir / "config.json").native();
  std::string invalid_config_path = (tmp_dir / "invalid_config.json").native();
  std::string damaged_config_path = (tmp_dir / "damaged_config.json").native();
  boost::filesystem::path base_path = tmp_dir / "base_path";
  boost::filesystem::path chain_path = tmp_dir / "genesis.json";

  static constexpr char const *file_content =
      R"({
        "general" : {
          "verbosity" : 2
        },
        "blockchain" : {
          "chain" : "%1%"
        },
        "storage" : {
          "base-path" : "%2%"
        },
        "network" : {
              "port" : 456,
              "rpc-host" : "1.1.1.1",
              "rpc-port" : 123,
              "ws-host" : "2.2.2.2",
              "ws-port" : 678
        },
        "additional" : {
          "single_finalizing_node" : true
        }
      })";
  static constexpr char const *invalid_file_content =
      R"({
        "general" : {
          "verbosity" : 4444
        },
        "blockchain" : {
          "chain" : 1
        },
        "storage" : {
          base_path.native().c_str() : 2
        },
        "network" : {
              "port" : "13",
              "rpc-host" : 7,
              "rpc-port" : "1312",
              "ws-host" : 5,
              "ws-port" : "AWESOME_PORT"
        },
        "additional" : {
          "single_finalizing_node" : "order1800"
        }
      })";
  static constexpr char const *damaged_file_content =
      R"({
        "general" : {
          "verbosity" : 4444
        },
        "blockchain" : {
          "chain" : 1
        },
        "storage" : nalizing_node" : "order1800"
        }
      })";

  boost::asio::ip::tcp::endpoint get_endpoint(char const *host, uint16_t port) {
    boost::asio::ip::tcp::endpoint endpoint;
    boost::system::error_code err;
    endpoint.address(boost::asio::ip::address::from_string(host, err));
    endpoint.port(port);
    return endpoint;
  }

  void SetUp() override {
    boost::filesystem::create_directory(tmp_dir);
    ASSERT_TRUE(boost::filesystem::exists(tmp_dir));

    auto spawn_file = [](std::string const &path,
                         std::string const &file_content) {
      std::ofstream file(path, std::ofstream::out | std::ofstream::trunc);
      file << file_content;
    };

    spawn_file(config_path,
               (boost::format(file_content) % chain_path.native()
                % base_path.native())
                   .str());
    spawn_file(invalid_config_path, invalid_file_content);
    spawn_file(damaged_config_path, damaged_file_content);
    spawn_file(chain_path.native(), "");
    ASSERT_TRUE(boost::filesystem::create_directory(base_path));

    auto logger = kagome::log::createLogger("AppConfigTest", "testing");
    app_config_ = std::make_shared<AppConfigurationImpl>(logger);
  }

  void TearDown() override {
    app_config_.reset();
  }

  std::shared_ptr<AppConfigurationImpl> app_config_;
};

/**
 * @given new created AppConfigurationImpl
 * @when no arguments provided
 * @then only default values are available
 */
TEST_F(AppConfigurationTest, DefaultValuesTest) {
  boost::asio::ip::tcp::endpoint const http_endpoint =
      get_endpoint("0.0.0.0", 9933);
  boost::asio::ip::tcp::endpoint const ws_endpoint =
      get_endpoint("0.0.0.0", 9944);
  char const *args[] = {"/path/",
                        "--chain",
                        chain_path.native().c_str(),
                        "--base-path",
                        base_path.native().c_str()};

  ASSERT_TRUE(app_config_->initialize_from_args(
      AppConfiguration::LoadScheme::kValidating,
      sizeof(args) / sizeof(args[0]),
      (char **)args));

  ASSERT_EQ(app_config_->p2pPort(), 30363);
  ASSERT_EQ(app_config_->rpcHttpEndpoint(), http_endpoint);
  ASSERT_EQ(app_config_->rpcWsEndpoint(), ws_endpoint);
  ASSERT_EQ(app_config_->verbosity(), kagome::log::Level::INFO);
  ASSERT_EQ(app_config_->isOnlyFinalizing(), false);
}

/**
 * @given new created AppConfigurationImpl
 * @when correct endpoint data provided
 * @then we must receive correct endpoints on call
 */
TEST_F(AppConfigurationTest, EndpointsTest) {
  boost::asio::ip::tcp::endpoint const http_endpoint =
      get_endpoint("1.2.3.4", 1111);
  boost::asio::ip::tcp::endpoint const ws_endpoint =
      get_endpoint("5.6.7.8", 2222);
  char const *args[] = {
      "/path/",
      "--chain",
      chain_path.native().c_str(),
      "--base-path",
      base_path.native().c_str(),
      "--rpc-host",
      "1.2.3.4",
      "--ws-host",
      "5.6.7.8",
      "--rpc-port",
      "1111",
      "--ws-port",
      "2222",
  };

  ASSERT_TRUE(app_config_->initialize_from_args(
      AppConfiguration::LoadScheme::kValidating,
      sizeof(args) / sizeof(args[0]),
      (char **)args));

  ASSERT_EQ(app_config_->rpcHttpEndpoint(), http_endpoint);
  ASSERT_EQ(app_config_->rpcWsEndpoint(), ws_endpoint);
}

/**
 * @given new created AppConfigurationImpl
 * @when --chain cmd line arg is provided
 * @then we must receive this value from chainSpecPath() call
 */
TEST_F(AppConfigurationTest, GenesisPathTest) {
  char const *args[] = {"/path/",
                        "--chain",
                        chain_path.native().c_str(),
                        "--base-path",
                        base_path.native().c_str()};
  ASSERT_TRUE(app_config_->initialize_from_args(
      AppConfiguration::LoadScheme::kValidating,
      sizeof(args) / sizeof(args[0]),
      (char **)args));

  ASSERT_EQ(app_config_->chainSpecPath(), chain_path.native().c_str());
}

/**
 * @given new created AppConfigurationImpl
 * @when correct endpoint data provided in config file and in cmd line args
 * @then we must select cmd line version
 */
TEST_F(AppConfigurationTest, CrossConfigTest) {
  boost::asio::ip::tcp::endpoint const http_endpoint =
      get_endpoint("1.2.3.4", 1111);
  boost::asio::ip::tcp::endpoint const ws_endpoint =
      get_endpoint("5.6.7.8", 2222);
  char const *args[] = {
      "/path/",
      "--config-file",
      config_path.c_str(),
      "--rpc-host",
      "1.2.3.4",
      "--ws-host",
      "5.6.7.8",
      "--rpc-port",
      "1111",
      "--ws-port",
      "2222",
  };

  ASSERT_TRUE(app_config_->initialize_from_args(
      AppConfiguration::LoadScheme::kValidating,
      sizeof(args) / sizeof(args[0]),
      (char **)args));

  ASSERT_EQ(app_config_->rpcHttpEndpoint(), http_endpoint);
  ASSERT_EQ(app_config_->rpcWsEndpoint(), ws_endpoint);
}

/**
 * @given new created AppConfigurationImpl
 * @when --config_file cmd line arg is provided
 * @then we must put to config data from file
 */
TEST_F(AppConfigurationTest, ConfigFileTest) {
  boost::asio::ip::tcp::endpoint const http_endpoint =
      get_endpoint("1.1.1.1", 123);
  boost::asio::ip::tcp::endpoint const ws_endpoint =
      get_endpoint("2.2.2.2", 678);

  char const *args[] = {"/path/", "--config-file", config_path.c_str()};
  ASSERT_TRUE(app_config_->initialize_from_args(
      AppConfiguration::LoadScheme::kValidating,
      sizeof(args) / sizeof(args[0]),
      (char **)args));

  ASSERT_EQ(app_config_->chainSpecPath(), chain_path);
  ASSERT_EQ(app_config_->keystorePath("test_chain42"),
            base_path / "test_chain42/keystore");
  ASSERT_EQ(app_config_->databasePath("test_chain42"),
            base_path / "test_chain42/db");
  ASSERT_EQ(app_config_->p2pPort(), 456);
  ASSERT_EQ(app_config_->rpcHttpEndpoint(), http_endpoint);
  ASSERT_EQ(app_config_->rpcWsEndpoint(), ws_endpoint);
  ASSERT_EQ(app_config_->verbosity(), kagome::log::Level::DEBUG);
  ASSERT_EQ(app_config_->isOnlyFinalizing(), true);
}

/**
 * @given new created AppConfigurationImpl
 * @when --config_file cmd line arg is provided and data in config is not
 * correct
 * @then we must receive default values
 */
TEST_F(AppConfigurationTest, InvalidConfigFileTest) {
  boost::asio::ip::tcp::endpoint const http_endpoint =
      get_endpoint("0.0.0.0", 9933);
  boost::asio::ip::tcp::endpoint const ws_endpoint =
      get_endpoint("0.0.0.0", 9944);

  char const *args[] = {"/path/",
                        "--base-path",
                        base_path.native().c_str(),
                        "--chain",
                        chain_path.native().c_str(),
                        "--config-file",
                        invalid_config_path.c_str()};
  ASSERT_TRUE(app_config_->initialize_from_args(
      AppConfiguration::LoadScheme::kValidating,
      sizeof(args) / sizeof(args[0]),
      (char **)args));

  ASSERT_EQ(app_config_->chainSpecPath(), chain_path.native().c_str());
  ASSERT_EQ(app_config_->keystorePath("test_chain42"),
            base_path / "test_chain42/keystore");
  ASSERT_EQ(app_config_->databasePath("test_chain42"),
            base_path / "test_chain42/db");
  ASSERT_EQ(app_config_->p2pPort(), 30363);
  ASSERT_EQ(app_config_->rpcHttpEndpoint(), http_endpoint);
  ASSERT_EQ(app_config_->rpcWsEndpoint(), ws_endpoint);
  ASSERT_EQ(app_config_->verbosity(), kagome::log::Level::INFO);
  ASSERT_EQ(app_config_->isOnlyFinalizing(), false);
}

/**
 * @given new created AppConfigurationImpl
 * @when --config_file cmd line arg is provided and data in config is damaged
 * @then we must receive default values
 */
TEST_F(AppConfigurationTest, DamagedConfigFileTest) {
  boost::asio::ip::tcp::endpoint const http_endpoint =
      get_endpoint("0.0.0.0", 9933);
  boost::asio::ip::tcp::endpoint const ws_endpoint =
      get_endpoint("0.0.0.0", 9944);

  char const *args[] = {"/path/",
                        "--base-path",
                        base_path.native().c_str(),
                        "--chain",
                        chain_path.native().c_str(),
                        "--config-file",
                        damaged_config_path.c_str()};
  ASSERT_TRUE(app_config_->initialize_from_args(
      AppConfiguration::LoadScheme::kValidating,
      sizeof(args) / sizeof(args[0]),
      (char **)args));

  ASSERT_EQ(app_config_->chainSpecPath(), chain_path.native().c_str());
  ASSERT_EQ(app_config_->keystorePath("test_chain42"),
            base_path / "test_chain42/keystore");
  ASSERT_EQ(app_config_->databasePath("test_chain42"),
            base_path / "test_chain42/db");
  ASSERT_EQ(app_config_->p2pPort(), 30363);
  ASSERT_EQ(app_config_->rpcHttpEndpoint(), http_endpoint);
  ASSERT_EQ(app_config_->rpcWsEndpoint(), ws_endpoint);
  ASSERT_EQ(app_config_->verbosity(), kagome::log::Level::INFO);
  ASSERT_EQ(app_config_->isOnlyFinalizing(), false);
}

/**
 * @given new created AppConfigurationImpl
 * @when --config_file cmd line arg is provided argument is not correct
 * @then we must receive default values
 */
TEST_F(AppConfigurationTest, NoConfigFileTest) {
  boost::asio::ip::tcp::endpoint const http_endpoint =
      get_endpoint("0.0.0.0", 9933);
  boost::asio::ip::tcp::endpoint const ws_endpoint =
      get_endpoint("0.0.0.0", 9944);

  char const *args[] = {"/path/",
                        "--base-path",
                        base_path.native().c_str(),
                        "--chain",
                        chain_path.native().c_str(),
                        "--config-file",
                        "<some_file>"};
  ASSERT_TRUE(app_config_->initialize_from_args(
      AppConfiguration::LoadScheme::kValidating,
      sizeof(args) / sizeof(args[0]),
      (char **)args));

  ASSERT_EQ(app_config_->chainSpecPath(), chain_path.native().c_str());
  ASSERT_EQ(app_config_->keystorePath("test_chain42"),
            base_path / "test_chain42/keystore");
  ASSERT_EQ(app_config_->databasePath("test_chain42"),
            base_path / "test_chain42/db");
  ASSERT_EQ(app_config_->p2pPort(), 30363);
  ASSERT_EQ(app_config_->rpcHttpEndpoint(), http_endpoint);
  ASSERT_EQ(app_config_->rpcWsEndpoint(), ws_endpoint);
  ASSERT_EQ(app_config_->verbosity(), kagome::log::Level::INFO);
  ASSERT_EQ(app_config_->isOnlyFinalizing(), false);
}

/**
 * @given new created AppConfigurationImpl
 * @when --single_finalizing_node cmd line arg is provided
 * @then we must receive this value from is_single_finalizing_node() call
 */
TEST_F(AppConfigurationTest, OnlyFinalizeTest) {
  char const *args[] = {
      "/path/",
      "--single_finalizing_node",
      "true",
      "--chain",
      chain_path.native().c_str(),
      "--base-path",
      base_path.native().c_str(),
  };
  ASSERT_TRUE(app_config_->initialize_from_args(
      AppConfiguration::LoadScheme::kValidating,
      sizeof(args) / sizeof(args[0]),
      (char **)args));

  ASSERT_EQ(app_config_->isOnlyFinalizing(), true);
}

/**
 * @given new created AppConfigurationImpl
 * @when --base-path cmd line arg is provided
 * @then we must receive this value from base_path() call
 */
TEST_F(AppConfigurationTest, KeystorePathTest) {
  char const *args[] = {"/path/",
                        "--chain",
                        chain_path.native().c_str(),
                        "--base-path",
                        base_path.native().c_str()};
  ASSERT_TRUE(app_config_->initialize_from_args(
      AppConfiguration::LoadScheme::kValidating,
      sizeof(args) / sizeof(args[0]),
      (char **)args));

  ASSERT_EQ(app_config_->keystorePath("test_chain42"),
            base_path / "test_chain42/keystore");
  ASSERT_EQ(app_config_->databasePath("test_chain42"),
            base_path / "test_chain42/db");
}

/**
 * @given new created AppConfigurationImpl
 * @when --base-path cmd line arg is provided
 * @then we must receive this value from base_path() call
 */
TEST_F(AppConfigurationTest, base_pathPathTest) {
  char const *args[] = {"/path/",
                        "--chain",
                        chain_path.native().c_str(),
                        "--base-path",
                        base_path.native().c_str()};
  ASSERT_TRUE(app_config_->initialize_from_args(
      AppConfiguration::LoadScheme::kValidating,
      sizeof(args) / sizeof(args[0]),
      (char **)args));

  ASSERT_EQ(app_config_->keystorePath("test_chain42"),
            base_path / "test_chain42/keystore");
  ASSERT_EQ(app_config_->databasePath("test_chain42"),
            base_path / "test_chain42/db");
}

/**
 * @given new created AppConfigurationImpl
 * @when verbosity provided with value 1
 * @then we expect verbosity in config equal 'debug' and so on equal
 * log::Level
 */
TEST_F(AppConfigurationTest, VerbosityCmdLineTest) {
  {
    char const *args[] = {
        "/path/",
        "--verbosity",
        "0",
        "--chain",
        chain_path.native().c_str(),
        "--base-path",
        base_path.native().c_str(),
    };
    ASSERT_TRUE(app_config_->initialize_from_args(
        AppConfiguration::LoadScheme::kValidating,
        sizeof(args) / sizeof(args[0]),
        (char **)args));
    ASSERT_EQ(app_config_->verbosity(), kagome::log::Level::INFO);
  }
  {
    char const *args[] = {
        "/path/",
        "--verbosity",
        "1",
        "--chain",
        chain_path.native().c_str(),
        "--base-path",
        base_path.native().c_str(),
    };
    ASSERT_TRUE(app_config_->initialize_from_args(
        AppConfiguration::LoadScheme::kValidating,
        sizeof(args) / sizeof(args[0]),
        (char **)args));
    ASSERT_EQ(app_config_->verbosity(), kagome::log::Level::VERBOSE);
  }
  {
    char const *args[] = {
        "/path/",
        "--verbosity",
        "2",
        "--chain",
        chain_path.native().c_str(),
        "--base-path",
        base_path.native().c_str(),
    };
    ASSERT_TRUE(app_config_->initialize_from_args(
        AppConfiguration::LoadScheme::kValidating,
        sizeof(args) / sizeof(args[0]),
        (char **)args));
    ASSERT_EQ(app_config_->verbosity(), kagome::log::Level::DEBUG);
  }
  {
    char const *args[] = {
        "/path/",
        "--verbosity",
        "3",
        "--chain",
        chain_path.native().c_str(),
        "--base-path",
        base_path.native().c_str(),
    };
    ASSERT_TRUE(app_config_->initialize_from_args(
        AppConfiguration::LoadScheme::kValidating,
        sizeof(args) / sizeof(args[0]),
        (char **)args));
    ASSERT_EQ(app_config_->verbosity(), kagome::log::Level::TRACE);
  }
}

/**
 * @given new created AppConfigurationImpl
 * @when verbosity provided with unexpected value
 * @then we expect last saved value(def. kagome::log::Level::INFO)
 */
TEST_F(AppConfigurationTest, UnexpVerbosityCmdLineTest) {
  char const *args[] = {"/path/",
                        "--verbosity",
                        "555",
                        "--chain",
                        chain_path.native().c_str(),
                        "--base-path",
                        base_path.native().c_str()};
  ASSERT_TRUE(app_config_->initialize_from_args(
      AppConfiguration::LoadScheme::kValidating,
      sizeof(args) / sizeof(args[0]),
      (char **)args));
  ASSERT_EQ(app_config_->verbosity(), kagome::log::Level::INFO);
}

/**
 * @given new created AppConfigurationImpl
 * @when is_only_finalize present
 * @then we should receve true from the call
 */
TEST_F(AppConfigurationTest, OnlyFinalizeTestTest) {
  char const *args[] = {"/path/",
                        "-f",
                        "--chain",
                        chain_path.native().c_str(),
                        "--base-path",
                        base_path.native().c_str()};
  ASSERT_TRUE(app_config_->initialize_from_args(
      AppConfiguration::LoadScheme::kValidating,
      sizeof(args) / sizeof(args[0]),
      (char **)args));
  ASSERT_EQ(app_config_->isOnlyFinalizing(), true);
}

/**
 * @given new created AppConfigurationImpl
 * @when is_only_finalize present
 * @then we should receve true from the call
 */
TEST_F(AppConfigurationTest, OnlyFinalizeTestTest_2) {
  char const *args[] = {"/path/",
                        "--single_finalizing_node",
                        "--chain",
                        chain_path.native().c_str(),
                        "--base-path",
                        base_path.native().c_str()};
  ASSERT_TRUE(app_config_->initialize_from_args(
      AppConfiguration::LoadScheme::kValidating,
      sizeof(args) / sizeof(args[0]),
      (char **)args));
  ASSERT_EQ(app_config_->isOnlyFinalizing(), true);
}
