/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/assert.hpp>
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
          "roles": "full",
          "log": "debug"
        },
        "blockchain" : {
          "chain" : "%1%"
        },
        "storage" : {
          "base-path" : "%2%"
        },
        "network" : {
              "port" : 2345,
              "rpc-host" : "1.1.1.1",
              "rpc-port" : 1234,
              "ws-host" : "2.2.2.2",
              "ws-port" : 3456,
              "name" : "Bob's node",
              "telemetry-endpoints": [
                  "ws://localhost/submit 0",
                  "wss://telemetry.soramitsu.co.jp/submit 4"
              ]
        },
        "additional" : {
          "single-finalizing-node" : true
        }
      })";
  static constexpr char const *invalid_file_content =
      R"({
        "general" : {
          "roles": "azaza",
          "log": "invalid"
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
          "single-finalizing-node" : "order1800"
        }
      })";
  static constexpr char const *damaged_file_content =
      R"({
        "general" : {
          "roles": "full",
          "log": "debug"
        },
        "blockchain" : {
          "chain" : 1
        },
        "storage" : nalizing-node" : "order1800"
        }
      })";

  boost::asio::ip::tcp::endpoint get_endpoint(char const *host, uint16_t port) {
    boost::asio::ip::tcp::endpoint endpoint;
    boost::system::error_code err;
    endpoint.address(boost::asio::ip::address::from_string(host, err));
    endpoint.port(port);
    return endpoint;
  }

  kagome::telemetry::TelemetryEndpoint get_telemetry_endpoint(
      char const *endpoint_uri, uint8_t verbosity_level) {
    auto uri = kagome::common::Uri::parse(endpoint_uri);
    BOOST_VERIFY(not uri.error().has_value());
    return {std::move(uri), verbosity_level};
  }

  void SetUp() override {
    boost::filesystem::create_directory(tmp_dir);
    ASSERT_TRUE(boost::filesystem::exists(tmp_dir));

    auto spawn_file = [](std::string const &path,
                         std::string const &file_content) {
      std::ofstream file(path, std::ofstream::out | std::ofstream::trunc);
      file << file_content;
    };

    spawn_file(
        config_path,
        (boost::format(file_content) % chain_path.native() % base_path.native())
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

  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));

  ASSERT_EQ(app_config_->p2pPort(), 30363);
  ASSERT_EQ(app_config_->rpcHttpEndpoint(), http_endpoint);
  ASSERT_EQ(app_config_->rpcWsEndpoint(), ws_endpoint);
  ASSERT_EQ(app_config_->log(), std::vector<std::string>());
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

  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));

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
  ASSERT_TRUE(app_config_->initializeFromArgs(

      std::size(args), args));

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

  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));

  ASSERT_EQ(app_config_->rpcHttpEndpoint(), http_endpoint);
  ASSERT_EQ(app_config_->rpcWsEndpoint(), ws_endpoint);
}

/**
 * @given new created AppConfigurationImpl
 * @when correct telemetry endpoints data provided in config file
 * @then endpoints are correctly initialized
 */
TEST_F(AppConfigurationTest, TelemetryEndpointsFromConfig) {
  std::vector reference = {
      get_telemetry_endpoint("ws://localhost/submit", 0),
      get_telemetry_endpoint("wss://telemetry.soramitsu.co.jp/submit", 4),
  };
  char const *args[] = {
      "/path/",
      "--config-file",
      config_path.c_str(),
  };

  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), (char **)args));
  ASSERT_EQ(app_config_->telemetryEndpoints(), reference);
}

/**
 * @given new created AppConfigurationImpl
 * @when --config_file cmd line arg is provided
 * @then we must put to config data from file
 */
TEST_F(AppConfigurationTest, ConfigFileTest) {
  boost::asio::ip::tcp::endpoint const http_endpoint =
      get_endpoint("1.1.1.1", 1234);
  boost::asio::ip::tcp::endpoint const ws_endpoint =
      get_endpoint("2.2.2.2", 3456);

  char const *args[] = {"/path/", "--config-file", config_path.c_str()};
  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));

  ASSERT_EQ(app_config_->chainSpecPath(), chain_path);
  ASSERT_EQ(app_config_->keystorePath("test_chain42"),
            base_path / "test_chain42/keystore");
  ASSERT_EQ(app_config_->databasePath("test_chain42"),
            base_path / "test_chain42/db");
  ASSERT_EQ(app_config_->p2pPort(), 2345);
  ASSERT_EQ(app_config_->rpcHttpEndpoint(), http_endpoint);
  ASSERT_EQ(app_config_->rpcWsEndpoint(), ws_endpoint);
  ASSERT_EQ(app_config_->log(), std::vector<std::string>{"debug"});
  ASSERT_EQ(app_config_->nodeName(), "Bob's node");
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
  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));

  ASSERT_EQ(app_config_->chainSpecPath(), chain_path.native().c_str());
  ASSERT_EQ(app_config_->keystorePath("test_chain42"),
            base_path / "test_chain42/keystore");
  ASSERT_EQ(app_config_->databasePath("test_chain42"),
            base_path / "test_chain42/db");
  ASSERT_EQ(app_config_->p2pPort(), 30363);
  ASSERT_EQ(app_config_->rpcHttpEndpoint(), http_endpoint);
  ASSERT_EQ(app_config_->rpcWsEndpoint(), ws_endpoint);
  ASSERT_EQ(app_config_->log(), std::vector<std::string>());
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
  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));

  ASSERT_EQ(app_config_->chainSpecPath(), chain_path.native().c_str());
  ASSERT_EQ(app_config_->keystorePath("test_chain42"),
            base_path / "test_chain42/keystore");
  ASSERT_EQ(app_config_->databasePath("test_chain42"),
            base_path / "test_chain42/db");
  ASSERT_EQ(app_config_->p2pPort(), 30363);
  ASSERT_EQ(app_config_->rpcHttpEndpoint(), http_endpoint);
  ASSERT_EQ(app_config_->rpcWsEndpoint(), ws_endpoint);
  ASSERT_EQ(app_config_->log(), std::vector<std::string>());
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
  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));

  ASSERT_EQ(app_config_->chainSpecPath(), chain_path.native().c_str());
  ASSERT_EQ(app_config_->keystorePath("test_chain42"),
            base_path / "test_chain42/keystore");
  ASSERT_EQ(app_config_->databasePath("test_chain42"),
            base_path / "test_chain42/db");
  ASSERT_EQ(app_config_->p2pPort(), 30363);
  ASSERT_EQ(app_config_->rpcHttpEndpoint(), http_endpoint);
  ASSERT_EQ(app_config_->rpcWsEndpoint(), ws_endpoint);
  ASSERT_EQ(app_config_->log(), std::vector<std::string>());
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
  ASSERT_TRUE(app_config_->initializeFromArgs(

      std::size(args), args));

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
  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));

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
        "--log",
        "info",
        "--chain",
        chain_path.native().c_str(),
        "--base-path",
        base_path.native().c_str(),
    };
    ASSERT_TRUE(
        app_config_->initializeFromArgs(std::size(args), args));
    ASSERT_EQ(app_config_->log(), std::vector<std::string>{"info"});
  }
  {
    char const *args[] = {
        "/path/",
        "--log",
        "verbose",
        "--chain",
        chain_path.native().c_str(),
        "--base-path",
        base_path.native().c_str(),
    };
    ASSERT_TRUE(
        app_config_->initializeFromArgs(std::size(args), args));
    ASSERT_EQ(app_config_->log(), std::vector<std::string>{"verbose"});
  }
  {
    char const *args[] = {
        "/path/",
        "--log",
        "debug",
        "--chain",
        chain_path.native().c_str(),
        "--base-path",
        base_path.native().c_str(),
    };
    ASSERT_TRUE(
        app_config_->initializeFromArgs(std::size(args), args));
    ASSERT_EQ(app_config_->log(), std::vector<std::string>{"debug"});
  }
  {
    char const *args[] = {
        "/path/",
        "--log",
        "trace",
        "--chain",
        chain_path.native().c_str(),
        "--base-path",
        base_path.native().c_str(),
    };
    ASSERT_TRUE(
        app_config_->initializeFromArgs(std::size(args), args));
    ASSERT_EQ(app_config_->log(), std::vector<std::string>{"trace"});
  }
}

/**
 * @given new created AppConfigurationImpl
 * @when verbosity provided with unexpected value
 * @then we expect last saved value(def. kagome::log::Level::INFO)
 */
TEST_F(AppConfigurationTest, UnexpVerbosityCmdLineTest) {
  char const *args[] = {"/path/",
                        "--log",
                        "",
                        "--chain",
                        chain_path.native().c_str(),
                        "--base-path",
                        base_path.native().c_str()};
  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));
  ASSERT_EQ(app_config_->log(), std::vector<std::string>{""});
}

/**
 * @given newly created AppConfigurationImpl
 * @when node name set in command line arguments
 * @then the name is correctly passed to configuration
 */
TEST_F(AppConfigurationTest, NodeNameAsCommandLineOption) {
  char const *args[] = {"/path/",
                        "--chain",
                        chain_path.native().c_str(),
                        "--base-path",
                        base_path.native().c_str(),
                        "--name",
                        "Alice's node"};
  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));
  ASSERT_EQ(app_config_->nodeName(), "Alice's node");
}

/**
 * @given newly created AppConfigurationImpl
 * @when single telemetry endpoint set in command line arguments
 * @then the endpoint and verbosity level is correctly passed to configuration
 */
TEST_F(AppConfigurationTest, SingleTelemetryCliArg) {
  const auto reference = get_telemetry_endpoint("ws://localhost/submit", 0);
  char const *args[] = {"/path/",
                        "--chain",
                        chain_path.native().c_str(),
                        "--base-path",
                        base_path.native().c_str(),
                        "--telemetry-url",
                        "ws://localhost/submit 0"};
  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), (char **)args));

  auto parsed_endpoints = app_config_->telemetryEndpoints();
  ASSERT_EQ(parsed_endpoints.size(), 1);
  ASSERT_EQ(parsed_endpoints[0], reference);
}

/**
 * @given newly created AppConfigurationImpl
 * @when several telemetry endpoints passed as command line argument
 * @then endpoints and verbosity levels are correctly passed to configuration
 */
TEST_F(AppConfigurationTest, MultipleTelemetryCliArgs) {
  std::vector reference = {
      get_telemetry_endpoint("ws://localhost/submit", 0),
      get_telemetry_endpoint("wss://telemetry.soramitsu.co.jp/submit", 4),
  };
  char const *args[] = {"/path/",
                        "--chain",
                        chain_path.native().c_str(),
                        "--base-path",
                        base_path.native().c_str(),
                        "--telemetry-url",
                        "ws://localhost/submit 0",
                        "wss://telemetry.soramitsu.co.jp/submit 4"};
  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), (char **)args));
  ASSERT_EQ(app_config_->telemetryEndpoints(), reference);
}
