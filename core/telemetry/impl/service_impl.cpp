/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "telemetry/impl/service_impl.hpp"

#include <regex>

#include <fmt/core.h>

#define RAPIDJSON_NO_SIZETYPEDEFINE
namespace rapidjson {
  using SizeType = size_t;
}
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <libp2p/basic/scheduler/asio_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/scheduler_impl.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/transport/tcp/bytes_counter.hpp>

#include "common/uri.hpp"
#include "telemetry/impl/connection_impl.hpp"
#include "telemetry/impl/telemetry_thread_pool.hpp"
#include "utils/pool_handler_ready_make.hpp"

namespace {
  std::string json2string(rapidjson::Document &document) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer writer(buffer);
    document.Accept(writer);
    return buffer.GetString();
  }
}  // namespace

namespace kagome::telemetry {

  TelemetryServiceImpl::TelemetryServiceImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      const application::AppConfiguration &app_configuration,
      const application::ChainSpec &chain_spec,
      const libp2p::Host &host,
      std::shared_ptr<const transaction_pool::TransactionPool> tx_pool,
      std::shared_ptr<storage::SpacedStorage> storage,
      PeerCount peer_count,
      TelemetryThreadPool &telemetry_thread_pool)
      : app_configuration_{app_configuration},
        chain_spec_{chain_spec},
        host_{host},
        tx_pool_{std::move(tx_pool)},
        buffer_storage_{storage->getSpace(storage::Space::kDefault)},
        peer_count_{std::move(peer_count)},
        io_context_{telemetry_thread_pool.io_context()},
        scheduler_{std::make_shared<libp2p::basic::SchedulerImpl>(
            std::make_shared<libp2p::basic::AsioSchedulerBackend>(
                telemetry_thread_pool.io_context()),
            libp2p::basic::Scheduler::Config{})},
        enabled_{app_configuration_.isTelemetryEnabled()},
        log_{log::createLogger("TelemetryService", "telemetry")} {
    BOOST_ASSERT(tx_pool_);
    BOOST_ASSERT(buffer_storage_);
    if (enabled_) {
      pool_handler_ = poolHandlerReadyMake(
          this, app_state_manager, telemetry_thread_pool, log_);
      app_state_manager->takeControl(*this);
    } else {
      SL_INFO(log_, "Telemetry disabled");
    }
  }

  bool TelemetryServiceImpl::tryStart() {
    message_pool_ = std::make_shared<MessagePool>(
        kTelemetryMessageMaxLengthBytes, kTelemetryMessagePoolSize);
    prepareGreetingMessage();
    auto chain_spec = chainSpecEndpoints();
    const auto &cli_config = app_configuration_.telemetryEndpoints();
    const auto &endpoints = cli_config.empty() ? chain_spec : cli_config;

    for (const auto &endpoint : endpoints) {
      auto connection = std::make_shared<TelemetryConnectionImpl>(
          io_context_,
          endpoint,
          // there is no way for connections to live longer than *this
          [&](std::shared_ptr<TelemetryConnection> conn) {
            if (not shutdown_requested_) {
              conn->send(connectedMessage());
              last_finalized_.reported = 0;
            }
          },
          message_pool_,
          scheduler_);
      connections_.emplace_back(std::move(connection));
    }

    for (auto &connection : connections_) {
      connection->connect();
    }
    frequent_timer_ = scheduler_->scheduleWithHandle(
        [&] { frequentNotificationsRoutine(); }, kTelemetryReportingInterval);
    delayed_timer_ = scheduler_->scheduleWithHandle(
        [&] { delayedNotificationsRoutine(); }, kTelemetrySystemReportInterval);
    return true;
  }

  void TelemetryServiceImpl::stop() {
    shutdown_requested_ = true;
    frequent_timer_.reset();
    delayed_timer_.reset();
    for (auto &connection : connections_) {
      connection->shutdown();
    }
  }

  std::vector<TelemetryEndpoint> TelemetryServiceImpl::chainSpecEndpoints()
      const {
    std::vector<TelemetryEndpoint> endpoints;
    auto &from_spec = chain_spec_.telemetryEndpoints();
    endpoints.reserve(from_spec.size());
    for (const auto &endpoint : from_spec) {
      // unfortunately, cannot use structured binding due to clang limitations
      auto uri_candidate = endpoint.first;

      if (not uri_candidate.empty() and '/' == uri_candidate.at(0)) {
        // assume endpoint specified as multiaddress
        auto ma_res = libp2p::multi::Multiaddress::create(uri_candidate);
        if (ma_res.has_error()) {
          SL_WARN(log_,
                  "Telemetry endpoint '{}' cannot be interpreted as a valid "
                  "multiaddress and was skipped due to error: {}",
                  uri_candidate,
                  ma_res.error());
          continue;
        }

        {
          // transform multiaddr of telemetry endpoint into uri form
          auto parts = ma_res.value().getProtocolsWithValues();
          if (parts.size() != 3) {
            SL_WARN(
                log_,
                "Telemetry endpoint '{}' has unknown format and was skipped",
                uri_candidate);
            continue;
          }
          auto host = parts[0].second;
          auto schema = parts[2].first.name.substr(std::strlen("x-parity-"));
          auto path =
              std::regex_replace(parts[2].second, std::regex("%2F"), "/");
          uri_candidate = fmt::format("{}://{}{}", schema, host, path);
        }
      }
      auto parsed_uri = common::Uri::parse(uri_candidate);
      if (parsed_uri.error().has_value()) {
        SL_WARN(log_,
                "Telemetry endpoint '{}' cannot be interpreted as a valid URI "
                "and was skipped due to error: {}",
                uri_candidate,
                parsed_uri.error().value());
        continue;
      }

      auto &verbosity = endpoint.second;
      if (verbosity > 9) {
        SL_WARN(log_,
                "Telemetry endpoint '{}' is not valid, its verbosity level is "
                "above the maximum possible {} > 9",
                uri_candidate,
                verbosity);
        continue;
      }
      endpoints.emplace_back(std::move(parsed_uri), verbosity);
    }
    return endpoints;
  }

  void TelemetryServiceImpl::frequentNotificationsRoutine() {
    frequent_timer_.reset();
    if (shutdown_requested_) {
      return;
    }
    std::optional<MessageHandle> last_imported_msg, last_finalized_msg;
    auto refs = connections_.size();
    {
      // do quick information retrieval under spin lock
      std::lock_guard lock(cache_mutex_);
      // prepare last imported block message
      if (last_imported_.is_set) {
        last_imported_.is_set = false;
        auto msg =
            blockNotification(last_imported_.block, last_imported_.origin);
        // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
        last_imported_msg = message_pool_->push(msg, refs);
      }
      // prepare last finalized message if there is a need to
      if (last_finalized_.reported < last_finalized_.block.number) {
        auto msg = blockNotification(last_finalized_.block, std::nullopt);
        // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
        last_finalized_msg = message_pool_->push(msg, refs);
        last_finalized_.reported = last_finalized_.block.number;
      }
    }
    for (auto &conn : connections_) {
      if (last_imported_msg) {
        conn->send(*last_imported_msg);
      }
      if (last_finalized_msg) {
        conn->send(*last_finalized_msg);
      }
    }
    frequent_timer_ = scheduler_->scheduleWithHandle(
        [&] { frequentNotificationsRoutine(); }, kTelemetryReportingInterval);
  }

  void TelemetryServiceImpl::delayedNotificationsRoutine() {
    delayed_timer_.reset();
    if (shutdown_requested_) {
      return;
    }
    std::optional<MessageHandle> system_msg_1, system_msg_2;
    auto refs = connections_.size();
    // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
    system_msg_1 = message_pool_->push(systemIntervalMessage1(), refs);
    // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
    system_msg_2 = message_pool_->push(systemIntervalMessage2(), refs);

    for (auto &conn : connections_) {
      if (system_msg_1) {
        conn->send(*system_msg_1);
      }
      if (system_msg_2) {
        conn->send(*system_msg_2);
      }
    }
    delayed_timer_ = scheduler_->scheduleWithHandle(
        [&] { delayedNotificationsRoutine(); }, kTelemetrySystemReportInterval);
  }

  void TelemetryServiceImpl::prepareGreetingMessage() {
    greeting_json_.SetObject();
    auto &allocator = greeting_json_.GetAllocator();
    auto str_val = [&allocator](const std::string &str) -> rapidjson::Value & {
      static rapidjson::Value val;
      val.SetString(str.c_str(),
                    static_cast<rapidjson::SizeType>(str.length()),
                    allocator);
      return val;
    };

    auto startup_time =
        std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count());

    rapidjson::Value payload(rapidjson::kObjectType);
    payload
        .AddMember(
            "authority", app_configuration_.roles().isAuthority(), allocator)
        .AddMember("chain", str_val(chain_spec_.name()), allocator)
        .AddMember("config", str_val(""), allocator)
        .AddMember("genesis_hash", str_val(genesis_hash_), allocator)
        .AddMember("implementation", str_val(kImplementationName), allocator)
        .AddMember("msg", str_val("system.connected"), allocator)
        .AddMember("name", str_val(app_configuration_.nodeName()), allocator)
        .AddMember("network_id", str_val(host_.getId().toBase58()), allocator)
        .AddMember("startup_time", str_val(startup_time), allocator)
        .AddMember(
            "version", str_val(app_configuration_.nodeVersion()), allocator);

    greeting_json_.AddMember("id", 1, allocator)
        .AddMember("payload", payload, allocator)
        .AddMember("ts", str_val(""), allocator);
  }

  std::string TelemetryServiceImpl::currentTimestamp() const {
    // UTC time works just fine.
    // The approach allows us just to append zero offset and avoid computation
    // of actual offset and modifying the offset string and timestamp itself.
    auto t = boost::posix_time::microsec_clock::universal_time();
    return boost::posix_time::to_iso_extended_string(t) + "+00:00";
  }

  std::string TelemetryServiceImpl::connectedMessage() {
    auto &allocator = greeting_json_.GetAllocator();
    auto timestamp_string = currentTimestamp();
    greeting_json_["ts"].SetString(
        timestamp_string.c_str(), timestamp_string.length(), allocator);

    return json2string(greeting_json_);
  }

  void TelemetryServiceImpl::notifyBlockImported(
      const primitives::BlockInfo &info, BlockOrigin origin) {
    if (not enabled_ or shutdown_requested_) {
      return;
    }
    std::lock_guard lock(cache_mutex_);
    last_imported_.block = info;
    last_imported_.origin = origin;
    last_imported_.is_set = true;
  }

  void TelemetryServiceImpl::notifyBlockFinalized(
      const primitives::BlockInfo &info) {
    if (not enabled_ or shutdown_requested_) {
      return;
    }
    if (info.number > last_finalized_.block.number) {
      std::lock_guard lock(cache_mutex_);
      last_finalized_.block = info;
    }
  }

  void TelemetryServiceImpl::pushBlockStats() {
    if (not enabled_) {
      return;
    }
    REINVOKE(*pool_handler_, pushBlockStats);
    frequentNotificationsRoutine();
  }

  std::string TelemetryServiceImpl::blockNotification(
      const primitives::BlockInfo &info, std::optional<BlockOrigin> origin) {
    std::string event_name =
        origin.has_value() ? "block.import" : "notify.finalized";

    rapidjson::Document document;
    document.SetObject();
    auto &allocator = document.GetAllocator();
    auto str_val = [&allocator](const std::string &str) -> rapidjson::Value & {
      static rapidjson::Value val;
      val.SetString(str.c_str(),
                    static_cast<rapidjson::SizeType>(str.length()),
                    allocator);
      return val;
    };

    rapidjson::Value payload(rapidjson::kObjectType), height;

    payload.AddMember(
        "best", str_val(fmt::format("{:l}", info.hash)), allocator);

    if (origin.has_value()) {
      // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
      height.SetInt(info.number);
      rapidjson::Value origin_val;
      using o = BlockOrigin;
      switch (origin.value()) {
        case o::kGenesis:
          origin_val = str_val("Genesis");
          break;
        case o::kNetworkInitialSync:
          origin_val = was_synchronized_ ? str_val("NetworkBroadcast")
                                         : str_val("NetworkInitialSync");
          break;
        case o::kNetworkBroadcast:
          origin_val = str_val("NetworkBroadcast");
          break;
        case o::kConsensusBroadcast:
          origin_val = str_val("ConsensusBroadcast");
          break;
        case o::kOwn:
          origin_val = str_val("Own");
          break;
        case o::kFile:
        default:
          origin_val = str_val("File");
      }
      payload.AddMember("origin", origin_val, allocator);
    } else {
      auto finalized_height_str = std::to_string(info.number);
      height.SetString(
          finalized_height_str.c_str(),
          static_cast<rapidjson::SizeType>(finalized_height_str.length()),
          allocator);
    }
    payload.AddMember("height", height, allocator)
        .AddMember("msg", str_val(event_name), allocator);

    document.AddMember("id", 1, allocator)
        .AddMember("payload", payload, allocator)
        .AddMember("ts", str_val(currentTimestamp()), allocator);

    return json2string(document);
  }

  std::string TelemetryServiceImpl::systemIntervalMessage1() {
    rapidjson::Document document;
    document.SetObject();
    auto &allocator = document.GetAllocator();
    auto str_val = [&allocator](const std::string &str) -> rapidjson::Value & {
      static rapidjson::Value val;
      val.SetString(str.c_str(),
                    static_cast<rapidjson::SizeType>(str.length()),
                    allocator);
      return val;
    };

    rapidjson::Value payload(rapidjson::kObjectType);

    rapidjson::Value height, finalized_height, tx_count, state_size, best_hash,
        finalized_hash;
    {
      std::lock_guard lock(cache_mutex_);
      // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
      height.SetInt(last_imported_.block.number);
      // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
      finalized_height.SetInt(last_finalized_.block.number);
      // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
      tx_count.SetInt(tx_pool_->getStatus().ready_num);
      // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
      state_size.SetInt(buffer_storage_->byteSizeHint().value_or(0));
      best_hash = str_val(fmt::format("{:l}", last_imported_.block.hash));
      finalized_hash = str_val(fmt::format("{:l}", last_finalized_.block.hash));
    }

    // fields order is preserved the same way substrate orders it
    payload.AddMember("best", best_hash, allocator)
        .AddMember("finalized_hash", finalized_hash, allocator)
        .AddMember("finalized_height", finalized_height, allocator)
        .AddMember("height", height, allocator)
        .AddMember("msg", str_val("system.interval"), allocator)
        .AddMember("txcount", tx_count, allocator)
        .AddMember("used_state_cache_size", state_size, allocator);

    document.AddMember("id", 1, allocator)
        .AddMember("payload", payload, allocator)
        .AddMember("ts", str_val(currentTimestamp()), allocator);

    return json2string(document);
  }

  std::string TelemetryServiceImpl::systemIntervalMessage2() {
    rapidjson::Document document;
    document.SetObject();
    auto &allocator = document.GetAllocator();
    auto str_val = [&allocator](const std::string &str) -> rapidjson::Value & {
      static rapidjson::Value val;
      val.SetString(str.c_str(),
                    static_cast<rapidjson::SizeType>(str.length()),
                    allocator);
      return val;
    };

    rapidjson::Value payload(rapidjson::kObjectType);

    rapidjson::Value peers_count;
    peers_count.SetUint(*peer_count_.v);

    auto bandwidth = getBandwidth();
    rapidjson::Value upBandwidth, downBandwidth;
    downBandwidth.SetUint64(bandwidth.down);
    upBandwidth.SetUint64(bandwidth.up);
    // fields order is preserved the same way substrate orders it
    payload.AddMember("bandwidth_download", downBandwidth, allocator)
        .AddMember("bandwidth_upload", upBandwidth, allocator)
        .AddMember("msg", str_val("system.interval"), allocator)
        .AddMember("peers", peers_count, allocator);

    document.AddMember("id", 1, allocator)
        .AddMember("payload", payload, allocator)
        .AddMember("ts", str_val(currentTimestamp()), allocator);

    return json2string(document);
  }

  void TelemetryServiceImpl::setGenesisBlockHash(
      const primitives::BlockHash &hash) {
    genesis_hash_ = fmt::format("{:l}", hash);
  }

  void TelemetryServiceImpl::notifyWasSynchronized() {
    was_synchronized_ = true;
  }

  bool TelemetryServiceImpl::isEnabled() const {
    return enabled_;
  }

  TelemetryServiceImpl::Bandwidth TelemetryServiceImpl::getBandwidth() {
    if (not previous_bandwidth_calculated_) {
      previous_bandwidth_calculated_ =
          std::chrono::high_resolution_clock::now();
    }

    auto calculateBandwidth = [](uint64_t &previousBytes,
                                 uint64_t totalBytes,
                                 auto &bandwidth,
                                 const std::chrono::seconds &timeElapsed) {
      const auto bytesDiff = totalBytes - previousBytes;
      if (const auto secondsElapsed = timeElapsed.count(); secondsElapsed > 0) {
        bandwidth = bytesDiff / secondsElapsed;
      } else {
        bandwidth = bytesDiff;
      }
      previousBytes = totalBytes;
    };

    const auto currentTime = std::chrono::high_resolution_clock::now();
    const auto timeElapsed = std::chrono::duration_cast<std::chrono::seconds>(
        currentTime - *previous_bandwidth_calculated_);

    Bandwidth bandwidth;
    const auto &bytesCounter = libp2p::transport::ByteCounter::getInstance();
    calculateBandwidth(previous_bytes_read_,
                       bytesCounter.getBytesRead(),
                       bandwidth.down,
                       timeElapsed);

    calculateBandwidth(previous_bytes_written_,
                       bytesCounter.getBytesWritten(),
                       bandwidth.up,
                       timeElapsed);

    previous_bandwidth_calculated_ = currentTime;

    return bandwidth;
  }
}  // namespace kagome::telemetry
