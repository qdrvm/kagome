/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/assert.hpp>
#include <boost/format.hpp>
#include <fstream>

#include "application/impl/app_configuration_impl.hpp"
#include "filesystem/common.hpp"
#include "log/logger.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::application::AppConfiguration;
using kagome::application::AppConfigurationImpl;
namespace filesystem = kagome::filesystem;

class AppConfigurationTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  filesystem::path tmp_dir =
      filesystem::temp_directory_path() / filesystem::unique_path();
  std::string config_path = (tmp_dir / "config.json").native();
  std::string invalid_config_path = (tmp_dir / "invalid_config.json").native();
  std::string damaged_config_path = (tmp_dir / "damaged_config.json").native();
  filesystem::path base_path = tmp_dir / "base_path";
  filesystem::path chain_path = tmp_dir / "genesis.json";

  static constexpr const char *file_content =
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
              "rpc-host" : "2.2.2.2",
              "rpc-port" : 3456,
              "name" : "Bob's node",
              "telemetry-endpoints": [
                  "ws://localhost/submit 0",
                  "wss://telemetry.soramitsu.co.jp/submit 4"
              ],
              "random-walk-interval" : 30
        },
        "additional" : {
          "single-finalizing-node" : true
        }
      })";
  static constexpr const char *invalid_file_content =
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
              "rpc-port" : "1312"
        },
        "additional" : {
          "single-finalizing-node" : "order1800"
        }
      })";
  static constexpr const char *damaged_file_content =
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

  boost::asio::ip::tcp::endpoint get_endpoint(const char *host, uint16_t port) {
    boost::asio::ip::tcp::endpoint endpoint;
    boost::system::error_code err;
    endpoint.address(boost::asio::ip::make_address(host, err));
    endpoint.port(port);
    return endpoint;
  }

  kagome::telemetry::TelemetryEndpoint get_telemetry_endpoint(
      const char *endpoint_uri, uint8_t verbosity_level) {
    auto uri = kagome::common::Uri::parse(endpoint_uri);
    BOOST_VERIFY(not uri.error().has_value());
    return {std::move(uri), verbosity_level};
  }

  void SetUp() override {
    filesystem::create_directory(tmp_dir);
    ASSERT_TRUE(filesystem::exists(tmp_dir));

    auto spawn_file = [](const std::string &path,
                         const std::string &file_content) {
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
    ASSERT_TRUE(filesystem::create_directory(base_path));

    app_config_ = std::make_shared<AppConfigurationImpl>();
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
  const boost::asio::ip::tcp::endpoint ws_endpoint =
      get_endpoint("0.0.0.0", 9944);
  const char *args[] = {"/path/",
                        "--chain",
                        chain_path.native().c_str(),
                        "--base-path",
                        base_path.native().c_str()};

  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));

  ASSERT_EQ(app_config_->p2pPort(), 30363);
  ASSERT_EQ(app_config_->rpcEndpoint(), ws_endpoint);
  ASSERT_EQ(app_config_->log(), std::vector<std::string>());
}

/**
 * @given new created AppConfigurationImpl
 * @when correct endpoint data provided
 * @then we must receive correct endpoints on call
 */
TEST_F(AppConfigurationTest, EndpointsTest) {
  const boost::asio::ip::tcp::endpoint ws_endpoint =
      get_endpoint("5.6.7.8", 2222);
  const char *args[] = {
      "/path/",
      "--chain",
      chain_path.native().c_str(),
      "--base-path",
      base_path.native().c_str(),
      "--rpc-host",
      "5.6.7.8",
      "--rpc-port",
      "2222",
  };

  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));

  ASSERT_EQ(app_config_->rpcEndpoint(), ws_endpoint);
}

/**
 * @given new created AppConfigurationImpl
 * @when --chain cmd line arg is provided
 * @then we must receive this value from chainSpecPath() call
 */
TEST_F(AppConfigurationTest, GenesisPathTest) {
  const char *args[] = {"/path/",
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
  const boost::asio::ip::tcp::endpoint ws_endpoint =
      get_endpoint("5.6.7.8", 2222);
  const char *args[] = {
      "/path/",
      "--config-file",
      config_path.c_str(),
      "--rpc-host",
      "5.6.7.8",
      "--rpc-port",
      "2222",
  };

  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));

  ASSERT_EQ(app_config_->rpcEndpoint(), ws_endpoint);
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
  const char *args[] = {
      "/path/",
      "--config-file",
      config_path.c_str(),
  };

  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));
  ASSERT_EQ(app_config_->telemetryEndpoints(), reference);
}

/**
 * @given an instance of AppConfigurationImpl
 * @when telemetry disabling flag is not passed
 * @then telemetry broadcasting considered to be enabled
 */
TEST_F(AppConfigurationTest, TelemetryDefaultlyEnabled) {
  const char *args[] = {
      "/path/",
      "--config-file",
      config_path.c_str(),
  };

  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));
  ASSERT_TRUE(app_config_->isTelemetryEnabled());
}

/**
 * @given an instance of AppConfigurationImpl
 * @when --no-telemetry flag is specified
 * @then telemetry broadcasting reported to be disabled
 */
TEST_F(AppConfigurationTest, TelemetryExplicitlyDisabled) {
  const char *args[] = {
      "/path/",
      "--config-file",
      config_path.c_str(),
      "--no-telemetry",
  };

  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));
  ASSERT_FALSE(app_config_->isTelemetryEnabled());
}

/**
 * @given an instance of AppConfigurationImpl
 * @when database configured to use RocksDB
 * @then RocksDB storage backend is going to be used
 */
TEST_F(AppConfigurationTest, RocksDBStorageBackend) {
  const char *args[] = {
      "/path/",
      "--config-file",
      config_path.c_str(),
      "--database",
      "rocksdb",
  };

  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));
  ASSERT_EQ(AppConfiguration::StorageBackend::RocksDB,
            app_config_->storageBackend());
}

/**
 * @given new created AppConfigurationImpl
 * @when --config_file cmd line arg is provided
 * @then we must put to config data from file
 */
TEST_F(AppConfigurationTest, ConfigFileTest) {
  const boost::asio::ip::tcp::endpoint ws_endpoint =
      get_endpoint("2.2.2.2", 3456);

  const char *args[] = {"/path/", "--config-file", config_path.c_str()};
  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));

  ASSERT_EQ(app_config_->chainSpecPath(), chain_path);
  ASSERT_EQ(app_config_->keystorePath("test_chain42"),
            base_path / "chains/test_chain42/keystore");
  ASSERT_EQ(app_config_->databasePath("test_chain42"),
            base_path / "chains/test_chain42/db");
  ASSERT_EQ(app_config_->p2pPort(), 2345);
  ASSERT_EQ(app_config_->rpcEndpoint(), ws_endpoint);
  ASSERT_EQ(app_config_->log(), std::vector<std::string>{"debug"});
  ASSERT_EQ(app_config_->nodeName(), "Bob's node");
  ASSERT_EQ(app_config_->getRandomWalkInterval(), std::chrono::seconds(30));
}

/**
 * @given new created AppConfigurationImpl
 * @when --config_file cmd line arg is provided and data in config is not
 * correct
 * @then we must receive default values
 */
TEST_F(AppConfigurationTest, InvalidConfigFileTest) {
  const boost::asio::ip::tcp::endpoint ws_endpoint =
      get_endpoint("0.0.0.0", 9944);

  const char *args[] = {"/path/",
                        "--base-path",
                        base_path.native().c_str(),
                        "--chain",
                        chain_path.native().c_str(),
                        "--config-file",
                        invalid_config_path.c_str()};
  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));

  ASSERT_EQ(app_config_->chainSpecPath(), chain_path.native().c_str());
  ASSERT_EQ(app_config_->keystorePath("test_chain42"),
            base_path / "chains/test_chain42/keystore");
  ASSERT_EQ(app_config_->databasePath("test_chain42"),
            base_path / "chains/test_chain42/db");
  ASSERT_EQ(app_config_->p2pPort(), 30363);
  ASSERT_EQ(app_config_->rpcEndpoint(), ws_endpoint);
  ASSERT_EQ(app_config_->log(), std::vector<std::string>());
}

/**
 * @given new created AppConfigurationImpl
 * @when --config_file cmd line arg is provided and data in config is damaged
 * @then we must receive default values
 */
TEST_F(AppConfigurationTest, DamagedConfigFileTest) {
  const boost::asio::ip::tcp::endpoint ws_endpoint =
      get_endpoint("0.0.0.0", 9944);

  const char *args[] = {"/path/",
                        "--base-path",
                        base_path.native().c_str(),
                        "--chain",
                        chain_path.native().c_str(),
                        "--config-file",
                        damaged_config_path.c_str()};
  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));

  ASSERT_EQ(app_config_->chainSpecPath(), chain_path.native().c_str());
  ASSERT_EQ(app_config_->keystorePath("test_chain42"),
            base_path / "chains/test_chain42/keystore");
  ASSERT_EQ(app_config_->databasePath("test_chain42"),
            base_path / "chains/test_chain42/db");
  ASSERT_EQ(app_config_->p2pPort(), 30363);
  ASSERT_EQ(app_config_->rpcEndpoint(), ws_endpoint);
  ASSERT_EQ(app_config_->log(), std::vector<std::string>());
}

/**
 * @given new created AppConfigurationImpl
 * @when --config_file cmd line arg is provided argument is not correct
 * @then we must receive default values
 */
TEST_F(AppConfigurationTest, NoConfigFileTest) {
  const boost::asio::ip::tcp::endpoint ws_endpoint =
      get_endpoint("0.0.0.0", 9944);

  const char *args[] = {"/path/",
                        "--base-path",
                        base_path.native().c_str(),
                        "--chain",
                        chain_path.native().c_str(),
                        "--config-file",
                        "<some_file>"};
  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));

  ASSERT_EQ(app_config_->chainSpecPath(), chain_path.native().c_str());
  ASSERT_EQ(app_config_->keystorePath("test_chain42"),
            base_path / "chains/test_chain42/keystore");
  ASSERT_EQ(app_config_->databasePath("test_chain42"),
            base_path / "chains/test_chain42/db");
  ASSERT_EQ(app_config_->p2pPort(), 30363);
  ASSERT_EQ(app_config_->rpcEndpoint(), ws_endpoint);
  ASSERT_EQ(app_config_->log(), std::vector<std::string>());
}

/**
 * @given new created AppConfigurationImpl
 * @when --base-path cmd line arg is provided
 * @then we must receive this value from base_path() call
 */
TEST_F(AppConfigurationTest, KeystorePathTest) {
  const char *args[] = {"/path/",
                        "--chain",
                        chain_path.native().c_str(),
                        "--base-path",
                        base_path.native().c_str()};
  ASSERT_TRUE(app_config_->initializeFromArgs(

      std::size(args), args));

  ASSERT_EQ(app_config_->keystorePath("test_chain42"),
            base_path / "chains/test_chain42/keystore");
  ASSERT_EQ(app_config_->databasePath("test_chain42"),
            base_path / "chains/test_chain42/db");
}

/**
 * @given new created AppConfigurationImpl
 * @when --base-path cmd line arg is provided
 * @then we must receive this value from base_path() call
 */
TEST_F(AppConfigurationTest, base_pathPathTest) {
  const char *args[] = {"/path/",
                        "--chain",
                        chain_path.native().c_str(),
                        "--base-path",
                        base_path.native().c_str()};
  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));

  ASSERT_EQ(app_config_->keystorePath("test_chain42"),
            base_path / "chains/test_chain42/keystore");
  ASSERT_EQ(app_config_->databasePath("test_chain42"),
            base_path / "chains/test_chain42/db");
}

/**
 * @given new created AppConfigurationImpl
 * @when verbosity provided with value 1
 * @then we expect verbosity in config equal 'debug' and so on equal
 * log::Level
 */
TEST_F(AppConfigurationTest, VerbosityCmdLineTest) {
  {
    const char *args[] = {
        "/path/",
        "--log",
        "info",
        "--chain",
        chain_path.native().c_str(),
        "--base-path",
        base_path.native().c_str(),
    };
    ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));
    ASSERT_EQ(app_config_->log(), std::vector<std::string>{"info"});
  }
  {
    const char *args[] = {
        "/path/",
        "--log",
        "verbose",
        "--chain",
        chain_path.native().c_str(),
        "--base-path",
        base_path.native().c_str(),
    };
    ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));
    ASSERT_EQ(app_config_->log(), std::vector<std::string>{"verbose"});
  }
  {
    const char *args[] = {
        "/path/",
        "--log",
        "debug",
        "--chain",
        chain_path.native().c_str(),
        "--base-path",
        base_path.native().c_str(),
    };
    ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));
    ASSERT_EQ(app_config_->log(), std::vector<std::string>{"debug"});
  }
  {
    const char *args[] = {
        "/path/",
        "--log",
        "trace",
        "--chain",
        chain_path.native().c_str(),
        "--base-path",
        base_path.native().c_str(),
    };
    ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));
    ASSERT_EQ(app_config_->log(), std::vector<std::string>{"trace"});
  }
}

/**
 * @given new created AppConfigurationImpl
 * @when verbosity provided with unexpected value
 * @then we expect last saved value(def. kagome::log::Level::INFO)
 */
TEST_F(AppConfigurationTest, UnexpVerbosityCmdLineTest) {
  const char *args[] = {"/path/",
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
  const char *args[] = {"/path/",
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
  const char *args[] = {"/path/",
                        "--chain",
                        chain_path.native().c_str(),
                        "--base-path",
                        base_path.native().c_str(),
                        "--telemetry-url",
                        "ws://localhost/submit 0"};
  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));

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
  const char *args[] = {"/path/",
                        "--chain",
                        chain_path.native().c_str(),
                        "--base-path",
                        base_path.native().c_str(),
                        "--telemetry-url",
                        "ws://localhost/submit 0",
                        "wss://telemetry.soramitsu.co.jp/submit 4"};
  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));
  ASSERT_EQ(app_config_->telemetryEndpoints(), reference);
}

/**
 * @given initialized instance of AppConfigurationImpl
 * @when --max-blocks-in-response is specified
 * @then the correct value is parsed
 */
TEST_F(AppConfigurationTest, MaxBlocksInResponse) {
  const char *args[] = {"/path/",
                        "--chain",
                        chain_path.native().c_str(),
                        "--base-path",
                        base_path.native().c_str(),
                        "--max-blocks-in-response",
                        "122"};
  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));

  ASSERT_EQ(app_config_->maxBlocksInResponse(), 122);
}

/**
 * @given an instance of AppConfigurationImpl
 * @when --random-walk-interval flag is not specified
 * @then random walk has default value
 */
TEST_F(AppConfigurationTest, DefaultRandomWalk) {
  const char *args[] = {
      "/path/",
      "--chain",
      chain_path.native().c_str(),
      "--base-path",
      base_path.native().c_str(),
  };

  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));
  ASSERT_EQ(app_config_->getRandomWalkInterval(), std::chrono::seconds(15));
}

/**
 * @given an instance of AppConfigurationImpl
 * @when --random-walk-interval flag is specified with a value
 * @then random walk has the specified value
 */
TEST_F(AppConfigurationTest, SetRandomWalk) {
  const char *args[] = {"/path/",
                        "--chain",
                        chain_path.native().c_str(),
                        "--base-path",
                        base_path.native().c_str(),
                        "--random-walk-interval",
                        "30"};

  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));
  ASSERT_EQ(app_config_->getRandomWalkInterval(), std::chrono::seconds(30));
}

/**
 * @given an instance of AppConfigurationImpl
 * @when --db-cache flag is specified with a value
 * @then the value is correctly passed to the program
 */
TEST_F(AppConfigurationTest, SetDbCacheSize) {
  const char *args[] = {"/path/",
                        "--chain",
                        chain_path.native().c_str(),
                        "--base-path",
                        base_path.native().c_str(),
                        "--db-cache",
                        "30"};

  ASSERT_TRUE(app_config_->initializeFromArgs(std::size(args), args));
  ASSERT_EQ(app_config_->dbCacheSize(), 30);
}
