/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <fstream>

#include "application/impl/app_config_impl.hpp"

using kagome::application::AppConfiguration;
using kagome::application::AppConfigurationImpl;

class AppConfigurationTest : public testing::Test {
 public:
  static constexpr char const *file_content =
      R"({
        "general" : {
          "verbosity" : 2
        },
        "blockchain" : {
          "genesis" : "genesis file path"
        },
        "storage" : {
          "leveldb" : "leveldb file path"
        },
        "authority" : {
          "keystore" : "keystore file path"
        },
        "network" : {
              "p2p_port" : 456,
              "rpc_http_host" : "1.1.1.1",
              "rpc_http_port" : 123,
              "rpc_ws_host" : "2.2.2.2",
              "rpc_ws_port" : 678
        },
        "additional" : {
          "single_finalizing_node" : true
        }
      })";
  boost::filesystem::path tmp_dir = boost::filesystem::temp_directory_path()
                                    / boost::filesystem::unique_path();
  std::string config_path = (tmp_dir / "config.json").native();

  boost::asio::ip::tcp::endpoint get_endpoint(char const *host, uint16_t port) {
    boost::asio::ip::tcp::endpoint endpoint;
    boost::system::error_code err;
    endpoint.address(boost::asio::ip::address::from_string(host, err));
    endpoint.port(port);
    return std::move(endpoint);
  }

  void SetUp() override {
    boost::filesystem::create_directory(tmp_dir);
    std::ofstream file(config_path, std::ofstream::out | std::ofstream::trunc);
    file << file_content;

    ASSERT_TRUE(boost::filesystem::exists(tmp_dir));
    auto logger = kagome::common::createLogger("App config test");
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
      get_endpoint("0.0.0.0", 40363);
  boost::asio::ip::tcp::endpoint const ws_endpoint =
      get_endpoint("0.0.0.0", 40364);
  char const *args[] = {"/path/",
                        "--genesis",
                        "genesis_path",
                        "--leveldb",
                        "leveldb_path",
                        "--keystore",
                        "keystore path"};

  app_config_->initialize_from_args(AppConfiguration::LoadScheme::kValidating,
                                    sizeof(args) / sizeof(args[0]),
                                    (char **)args);

  ASSERT_EQ(app_config_->p2p_port(), 30363);
  ASSERT_EQ(app_config_->rpc_http_endpoint(), http_endpoint);
  ASSERT_EQ(app_config_->rpc_ws_endpoint(), ws_endpoint);
  ASSERT_EQ(app_config_->verbosity(), spdlog::level::level_enum::info);
  ASSERT_EQ(app_config_->is_only_finalizing(), false);
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
      "--genesis",
      "genesis_path",
      "--leveldb",
      "leveldb_path",
      "--keystore",
      "keystore path",
      "--rpc_http_host",
      "1.2.3.4",
      "--rpc_ws_host",
      "5.6.7.8",
      "--rpc_http_port",
      "1111",
      "--rpc_ws_port",
      "2222",
  };

  app_config_->initialize_from_args(AppConfiguration::LoadScheme::kValidating,
                                    sizeof(args) / sizeof(args[0]),
                                    (char **)args);

  ASSERT_EQ(app_config_->rpc_http_endpoint(), http_endpoint);
  ASSERT_EQ(app_config_->rpc_ws_endpoint(), ws_endpoint);
}

/**
 * @given new created AppConfigurationImpl
 * @when --genesis cmd line arg is provided
 * @then we must receive this value from genesis_path() call
 */
TEST_F(AppConfigurationTest, GenesisPathTest) {
  char const *args[] = {"/path/",
                        "--genesis",
                        "genesis_path",
                        "--leveldb",
                        "leveldb_path",
                        "--keystore",
                        "keystore path"};
  app_config_->initialize_from_args(AppConfiguration::LoadScheme::kValidating,
                                    sizeof(args) / sizeof(args[0]),
                                    (char **)args);

  ASSERT_EQ(app_config_->genesis_path(), "genesis_path");
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
      "--config_file",
      config_path.c_str(),
      "--rpc_http_host",
      "1.2.3.4",
      "--rpc_ws_host",
      "5.6.7.8",
      "--rpc_http_port",
      "1111",
      "--rpc_ws_port",
      "2222",
  };

  app_config_->initialize_from_args(AppConfiguration::LoadScheme::kValidating,
                                    sizeof(args) / sizeof(args[0]),
                                    (char **)args);

  ASSERT_EQ(app_config_->rpc_http_endpoint(), http_endpoint);
  ASSERT_EQ(app_config_->rpc_ws_endpoint(), ws_endpoint);
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

  char const *args[] = {"/path/", "--config_file", config_path.c_str()};
  app_config_->initialize_from_args(AppConfiguration::LoadScheme::kValidating,
                                    sizeof(args) / sizeof(args[0]),
                                    (char **)args);

  ASSERT_EQ(app_config_->genesis_path(), "genesis file path");
  ASSERT_EQ(app_config_->leveldb_path(), "leveldb file path");
  ASSERT_EQ(app_config_->keystore_path(), "keystore file path");
  ASSERT_EQ(app_config_->p2p_port(), 456);
  ASSERT_EQ(app_config_->rpc_http_endpoint(), http_endpoint);
  ASSERT_EQ(app_config_->rpc_ws_endpoint(), ws_endpoint);
  ASSERT_EQ(app_config_->verbosity(), spdlog::level::level_enum::info);
  ASSERT_EQ(app_config_->is_only_finalizing(), true);
}

/**
 * @given new created AppConfigurationImpl
 * @when --single_finalizing_node cmd line arg is provided
 * @then we must receive this value from is_single_finalizing_node() call
 */
TEST_F(AppConfigurationTest, OnlyFinalizeTest) {
  char const *args[] = {"/path/",
                        "--single_finalizing_node",
                        "true",
                        "--genesis",
                        "genesis_path",
                        "--leveldb",
                        "leveldb_path",
                        "--keystore",
                        "keystore path"};
  app_config_->initialize_from_args(AppConfiguration::LoadScheme::kValidating,
                                    sizeof(args) / sizeof(args[0]),
                                    (char **)args);

  ASSERT_EQ(app_config_->is_only_finalizing(), true);
}

/**
 * @given new created AppConfigurationImpl
 * @when --leveldb cmd line arg is provided
 * @then we must receive this value from leveldb_path() call
 */
TEST_F(AppConfigurationTest, KeystorePathTest) {
  char const *args[] = {"/path/",
                        "--keystore",
                        "keystore_path",
                        "--genesis",
                        "genesis_path",
                        "--leveldb",
                        "leveldb_path"};
  app_config_->initialize_from_args(AppConfiguration::LoadScheme::kValidating,
                                    sizeof(args) / sizeof(args[0]),
                                    (char **)args);

  ASSERT_EQ(app_config_->keystore_path(), "keystore_path");
}

/**
 * @given new created AppConfigurationImpl
 * @when --leveldb cmd line arg is provided
 * @then we must receive this value from leveldb_path() call
 */
TEST_F(AppConfigurationTest, LevelDBPathTest) {
  char const *args[] = {"/path/",
                        "--genesis",
                        "genesis_path",
                        "--leveldb",
                        "leveldb_path",
                        "--keystore",
                        "keystore path"};
  app_config_->initialize_from_args(AppConfiguration::LoadScheme::kValidating,
                                    sizeof(args) / sizeof(args[0]),
                                    (char **)args);

  ASSERT_EQ(app_config_->leveldb_path(), "leveldb_path");
}

/**
 * @given new created AppConfigurationImpl
 * @when verbosity provided with value 1
 * @then we expect verbosity in config equal 'debug' and so on equal
 * spdlog::level::level_enum
 */
TEST_F(AppConfigurationTest, VerbosityCmdLineTest) {
  {
    char const *args[] = {"/path/",
                          "--verbosity",
                          "0",
                          "--genesis",
                          "genesis_path",
                          "--leveldb",
                          "leveldb_path",
                          "--keystore",
                          "keystore path"};
    app_config_->initialize_from_args(AppConfiguration::LoadScheme::kValidating,
                                      sizeof(args) / sizeof(args[0]),
                                      (char **)args);
    ASSERT_EQ(app_config_->verbosity(), spdlog::level::level_enum::trace);
  }
  {
    char const *args[] = {"/path/",
                          "--verbosity",
                          "1",
                          "--genesis",
                          "genesis_path",
                          "--leveldb",
                          "leveldb_path",
                          "--keystore",
                          "keystore path"};
    app_config_->initialize_from_args(AppConfiguration::LoadScheme::kValidating,
                                      sizeof(args) / sizeof(args[0]),
                                      (char **)args);
    ASSERT_EQ(app_config_->verbosity(), spdlog::level::level_enum::debug);
  }
  {
    char const *args[] = {"/path/",
                          "--verbosity",
                          "2",
                          "--genesis",
                          "genesis_path",
                          "--leveldb",
                          "leveldb_path",
                          "--keystore",
                          "keystore path"};
    app_config_->initialize_from_args(AppConfiguration::LoadScheme::kValidating,
                                      sizeof(args) / sizeof(args[0]),
                                      (char **)args);
    ASSERT_EQ(app_config_->verbosity(), spdlog::level::level_enum::info);
  }
  {
    char const *args[] = {"/path/",
                          "--verbosity",
                          "3",
                          "--genesis",
                          "genesis_path",
                          "--leveldb",
                          "leveldb_path",
                          "--keystore",
                          "keystore path"};
    app_config_->initialize_from_args(AppConfiguration::LoadScheme::kValidating,
                                      sizeof(args) / sizeof(args[0]),
                                      (char **)args);
    ASSERT_EQ(app_config_->verbosity(), spdlog::level::level_enum::warn);
  }
  {
    char const *args[] = {"/path/",
                          "--verbosity",
                          "4",
                          "--genesis",
                          "genesis_path",
                          "--leveldb",
                          "leveldb_path",
                          "--keystore",
                          "keystore path"};
    app_config_->initialize_from_args(AppConfiguration::LoadScheme::kValidating,
                                      sizeof(args) / sizeof(args[0]),
                                      (char **)args);
    ASSERT_EQ(app_config_->verbosity(), spdlog::level::level_enum::err);
  }
  {
    char const *args[] = {"/path/",
                          "--verbosity",
                          "5",
                          "--genesis",
                          "genesis_path",
                          "--leveldb",
                          "leveldb_path",
                          "--keystore",
                          "keystore path"};
    app_config_->initialize_from_args(AppConfiguration::LoadScheme::kValidating,
                                      sizeof(args) / sizeof(args[0]),
                                      (char **)args);
    ASSERT_EQ(app_config_->verbosity(), spdlog::level::level_enum::critical);
  }
  {
    char const *args[] = {"/path/",
                          "--verbosity",
                          "6",
                          "--genesis",
                          "genesis_path",
                          "--leveldb",
                          "leveldb_path",
                          "--keystore",
                          "keystore path"};
    app_config_->initialize_from_args(AppConfiguration::LoadScheme::kValidating,
                                      sizeof(args) / sizeof(args[0]),
                                      (char **)args);
    ASSERT_EQ(app_config_->verbosity(), spdlog::level::level_enum::off);
  }
}

/**
 * @given new created AppConfigurationImpl
 * @when verbosity provided with unexpected value
 * @then we expect last saved value(def. spdlog::level::level_enum::info)
 */
TEST_F(AppConfigurationTest, UnexpVerbosityCmdLineTest) {
  char const *args[] = {"/path/",
                        "--verbosity",
                        "555",
                        "--genesis",
                        "genesis_path",
                        "--leveldb",
                        "leveldb_path",
                        "--keystore",
                        "keystore path"};
  app_config_->initialize_from_args(AppConfiguration::LoadScheme::kValidating,
                                    sizeof(args) / sizeof(args[0]),
                                    (char **)args);
  ASSERT_EQ(app_config_->verbosity(), spdlog::level::level_enum::info);
}