/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "state_metrics_impl.hpp"

#include "crypto/twox/twox.hpp"
#include "primitives/ss58_codec.hpp"
#include "utils/weak_macro.hpp"

namespace kagome::state_metrics {

  static constexpr auto SET_ERA_POINTS_PERIOD = 60;  // in seconds

  StateMetricsImpl::StateMetricsImpl(
      const application::AppConfiguration &app_config,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler,
      LazySPtr<api::StateApi> state_api,
      std::shared_ptr<metrics::Registry> registry,
      std::shared_ptr<crypto::Hasher> hasher)
      : scheduler_{std::move(scheduler)},
        state_api_{std::move(state_api)},
        stop_signal_received_{false},
        logger_{log::createLogger("StateMetrics", "state_metrics")} {
    BOOST_ASSERT(scheduler_);
    BOOST_ASSERT(registry);
    BOOST_ASSERT(hasher);
    auto validator_address_res = app_config.getValidatorAddress();
    if (not validator_address_res) {
      SL_INFO(logger_, "Validator address is not set, state metrics won't run");
      return;
    }
    const auto &validator_address = validator_address_res.value();
    auto validator_id = primitives::decodeSs58(validator_address, *hasher);
    if (not validator_id) {
      const auto error_message =
          std::string("Failed to decode validator address: ")
          + validator_id.error().message();
      throw std::runtime_error(error_message);
    }
    validator_id_ = std::move(validator_id.value());
    constexpr auto era_points_metric = "era_points";
    registry->registerGaugeFamily(
        era_points_metric,
        "The number of reward points for the active era for the validator",
        {{"validator_address", validator_address}});
    era_points_ = registry->registerGaugeMetric(era_points_metric);
    BOOST_ASSERT(era_points_);
    constexpr auto active_era_number_metric = "active_era_number";
    registry->registerGaugeFamily(active_era_number_metric,
                                  "The active era index",
                                  {{"validator_address", validator_address}});
    active_era_number_ =
        registry->registerGaugeMetric(active_era_number_metric);
    BOOST_ASSERT(active_era_number_);
    const auto staking_prefix =
        crypto::make_twox128(kagome::common::Buffer::fromString("Staking"));
    const auto active_era_prefix =
        crypto::make_twox128(kagome::common::Buffer::fromString("ActiveEra"));
    active_era_storage_key_.insert(active_era_storage_key_.end(),
                                   staking_prefix.begin(),
                                   staking_prefix.end());
    active_era_storage_key_.insert(active_era_storage_key_.end(),
                                   active_era_prefix.begin(),
                                   active_era_prefix.end());

    const auto reward_points_prefix = crypto::make_twox128(
        kagome::common::Buffer::fromString("ErasRewardPoints"));
    reward_points_storage_key_basis_.insert(
        reward_points_storage_key_basis_.end(),
        staking_prefix.begin(),
        staking_prefix.end());
    reward_points_storage_key_basis_.insert(
        reward_points_storage_key_basis_.end(),
        reward_points_prefix.begin(),
        reward_points_prefix.end());

    era_points_thread_ = std::thread([this] {
      for (;;) {
        scheduler_->schedule([WEAK_SELF] {
          WEAK_LOCK(self);
          self->updateEraPoints();
        });
        for (auto i = 0; i < SET_ERA_POINTS_PERIOD; ++i) {
          std::this_thread::sleep_for(std::chrono::seconds(1));
          if (stop_signal_received_.load()) {
            return;
          }
        }
      }
    });
  }

  StateMetricsImpl::~StateMetricsImpl() {
    stop_signal_received_.store(true);
    if (era_points_thread_.joinable()) {
      era_points_thread_.join();
    }
  }

  void StateMetricsImpl::updateEraPoints() {
    if (auto active_era = getActiveEraIndex()) {
      const auto &active_era_value = active_era.value();
      active_era_number_->set(active_era_value);
      if (auto reward_points = getRewardPoints(active_era_value)) {
        const auto &reward_points_value = reward_points.value();
        SL_TRACE(logger_,
                 "Reward points for era {}: {}",
                 active_era_value,
                 reward_points_value);
        era_points_->set(reward_points_value);
      }
    }
  }

  std::optional<uint32_t> StateMetricsImpl::getActiveEraIndex() {
    auto state_api = state_api_.get();
    if (state_api == nullptr) {
      return std::nullopt;
    }
    auto data = state_api->getStorage(active_era_storage_key_);
    if (not data) {
      SL_DEBUG(logger_,
               "Error while getting active era: {}",
               data.error().message());
      return std::nullopt;
    }
    const auto &opt_data = data.value();
    if (not opt_data) {
      SL_DEBUG(logger_, "Active era is not found");
      return std::nullopt;
    }
    scale::DecoderFromSpan decoder(opt_data.value());
    uint32_t active_era = 0;
    try {
      decoder >> active_era;
    } catch (const std::exception &e) {
      SL_DEBUG(logger_, "Error while decoding active era: {}", e.what());
      return std::nullopt;
    }
    return active_era;
  }

  std::vector<uint8_t> StateMetricsImpl::generateRewardPointsKey(
      uint32_t era_index) {
    const auto era_index_encoded_res = scale::encode(era_index);
    if (not era_index_encoded_res) {
      SL_DEBUG(logger_,
               "Error while encoding era index {}: {}",
               era_index,
               era_index_encoded_res.error().message());
      return {};
    }
    const auto &era_index_encoded = era_index_encoded_res.value();
    const auto hashed_era_index = crypto::make_twox64(era_index_encoded);
    auto storage_key = reward_points_storage_key_basis_;
    storage_key.insert(
        storage_key.end(), hashed_era_index.begin(), hashed_era_index.end());
    storage_key.insert(
        storage_key.end(), era_index_encoded.begin(), era_index_encoded.end());
    return storage_key;
  }

  std::optional<uint32_t> StateMetricsImpl::getRewardPoints(
      uint32_t era_index) {
    const auto storage_key = generateRewardPointsKey(era_index);
    auto state_api = state_api_.get();
    if (state_api == nullptr) {
      return std::nullopt;
    }
    const auto data = state_api->getStorage(storage_key);
    if (not data) {
      SL_DEBUG(logger_,
               "Error while getting reward points for era {}: {}",
               era_index,
               data.error().message());
      return std::nullopt;
    }
    const auto &opt_data = data.value();
    if (not opt_data) {
      SL_DEBUG(logger_, "Reward points are not found for era {}", era_index);
      return std::nullopt;
    }
    return parseErasRewardPoints(opt_data.value());
  }

  std::optional<uint32_t> StateMetricsImpl::parseErasRewardPoints(
      const std::vector<uint8_t> &data) try {
    scale::DecoderFromSpan decoder(data);

    uint32_t total_points = 0;
    decoder >> total_points;

    std::map<primitives::AccountId, uint32_t> individual_points;
    decoder >> individual_points;

    auto it = individual_points.find(validator_id_);
    if (it != individual_points.end()) {
      return it->second;
    }
    SL_TRACE(
        logger_, "Reward points are not found for validator {}", validator_id_);
    return 0;
  } catch (const std::exception &e) {
    SL_DEBUG(logger_, "Error while decoding reward points: {}", e.what());
    return std::nullopt;
  }
}  // namespace kagome::state_metrics
