/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "metrics_watcher.hpp"

#include "filesystem/common.hpp"

namespace {
  constexpr auto storageSizeMetricName = "kagome_storage_size";
}  // namespace

namespace kagome::metrics {
  namespace fs = kagome::filesystem;

  MetricsWatcher::MetricsWatcher(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      const application::AppConfiguration &app_config,
      std::shared_ptr<application::ChainSpec> chain_spec)
      : storage_path_(app_config.databasePath(chain_spec->id())) {
    BOOST_ASSERT(app_state_manager);

    // Metrics
    metrics_registry_ = metrics::createRegistry();

    metrics_registry_->registerGaugeFamily(
        storageSizeMetricName, "Consumption of disk space by storage");
    metric_storage_size_ =
        metrics_registry_->registerGaugeMetric(storageSizeMetricName);

    app_state_manager->takeControl(*this);
  }

  bool MetricsWatcher::start() {
    thread_ = std::thread([this] {
      soralog::util::setThreadName("metric-watcher");
      while (not shutdown_requested_) {
        auto storage_size_res = measure_storage_size();
        if (storage_size_res.has_value()) {
          metric_storage_size_->set(storage_size_res.value());
        }

        // Granulated waiting
        for (auto i = 0; i < 30; ++i) {
          std::this_thread::sleep_for(std::chrono::seconds(1));
          if (shutdown_requested_) {
            break;
          }
        }
      }
    });

    return true;
  }

  void MetricsWatcher::stop() {
    shutdown_requested_ = true;
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  outcome::result<uintmax_t> MetricsWatcher::measure_storage_size() {
    std::error_code ec;

    fs::directory_entry entry(storage_path_);

    if (not fs::is_directory(storage_path_, ec)) {
      if (ec) {
        return ec;
      }
      return std::errc::invalid_argument;
    }

    uintmax_t total_size = 0;
    for (fs::recursive_directory_iterator it(entry, ec);
         it != fs::recursive_directory_iterator();
         it.increment(ec)) {
      if (!ec) {
        const auto &dir_entry = *it;
        auto size = fs::file_size(dir_entry, ec);
        if (!ec) {
          total_size += size;
        }
      }
    }

    return outcome::success(total_size);
  }

}  // namespace kagome::metrics
