/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "metrics_watcher.hpp"

namespace {
  constexpr auto storageSizeMetricName = "kagome_storage_size";
}  // namespace

namespace kagome::metrics {
  namespace fs = boost::filesystem;

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

  bool MetricsWatcher::prepare() {
    return true;
  }

  bool MetricsWatcher::start() {
    thread_ = std::thread([this] {
      while (not shutdown_requested_) {
        auto storage_size_res = measure_storage_size();
        if (storage_size_res.has_value()) {
          metric_storage_size_->set(storage_size_res.value());
        }

        // Granulated waiting
        for (auto i = 0; i < 30; ++i) {
          std::this_thread::sleep_for(std::chrono::seconds(1));
          if (shutdown_requested_) break;
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

  outcome::result<size_t> MetricsWatcher::measure_storage_size() {
    boost::system::error_code ec;

    fs::directory_entry entry(storage_path_);

    if (not fs::is_directory(storage_path_, ec)) {
      if (ec) {
        return ec;
      }
      return boost::system::errc::invalid_argument;
    }

    size_t total_size = 0;

    std::function<void(const fs::directory_entry &,
                       boost::system::error_code &ec)>
        observe = [&](const fs::directory_entry &entry_arg,
                      boost::system::error_code &ec) {
          for (fs::directory_iterator it(entry_arg);
               it != fs::directory_iterator();
               ++it) {
            auto &entry = *it;

            auto size = fs::file_size(*it, ec);
            if (ec) {
              return;
            }
            total_size += size;

            auto is_dir = fs::is_directory(entry, ec);
            if (ec) {
              return;
            }
            if (is_dir) {
              observe(entry, ec);
            }
          }
        };

    observe(entry, ec);
    if (ec) {
      return ec;
    }

    return total_size;
  }

}  // namespace kagome::metrics
