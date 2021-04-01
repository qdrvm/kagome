/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_lottery_impl.hpp"

#include <unordered_set>

#include <boost/assert.hpp>
#include "common/buffer.hpp"
#include "common/mp_utils.hpp"
#include "consensus/validation/prepare_transcript.hpp"

namespace kagome::consensus {
  using common::Buffer;
  namespace vrf_constants = crypto::constants::sr25519::vrf;

  BabeLotteryImpl::BabeLotteryImpl(
      std::shared_ptr<crypto::VRFProvider> vrf_provider,
      std::shared_ptr<primitives::BabeConfiguration> configuration,
      std::shared_ptr<crypto::Hasher> hasher)
      : vrf_provider_{std::move(vrf_provider)},
        hasher_{std::move(hasher)},
        logger_{log::createLogger("BabeLottery", "babe_lottery")} {
    BOOST_ASSERT(vrf_provider_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(logger_);
    BOOST_ASSERT(configuration);
    epoch_length_ = configuration->epoch_length;
  }

  BabeLottery::SlotsLeadership BabeLotteryImpl::slotsLeadership(
      const EpochDescriptor &epoch,
      const Randomness &randomness,
      const Threshold &threshold,
      const crypto::Sr25519Keypair &keypair) const {
    BabeLottery::SlotsLeadership result;
    result.reserve(epoch_length_);

    for (BabeSlotNumber i = epoch.start_slot;
         i < epoch.start_slot + epoch_length_;
         ++i) {
      primitives::Transcript transcript;
      prepareTranscript(transcript, randomness, i, epoch.epoch_number);
      logger_->trace(
          "prepareTranscript (leadership): randomness {}, slot {}, epoch {}",
          randomness,
          i,
          epoch.epoch_number);

      auto sign_opt =
          vrf_provider_->signTranscript(transcript, keypair, threshold);

      result.push_back(sign_opt);
    }

    return result;
  }

  Randomness BabeLotteryImpl::computeRandomness(
      const Randomness &last_epoch_randomness, EpochNumber last_epoch_number) {
    static std::unordered_set<EpochNumber> computed_epochs_randomnesses{};

    // the function must never be called twice for the same epoch
    BOOST_ASSERT(computed_epochs_randomnesses.insert(last_epoch_number).second);

    // randomness || epoch_number || rho
    Buffer new_randomness(
        vrf_constants::OUTPUT_SIZE + 8 + last_epoch_vrf_values_.size() * 32, 0);

    std::copy(last_epoch_randomness.begin(),
              last_epoch_randomness.end(),
              new_randomness.begin());

    auto epoch_number_bytes = common::uint64_t_to_bytes(last_epoch_number);
    std::copy(epoch_number_bytes.begin(),
              epoch_number_bytes.end(),
              new_randomness.begin() + vrf_constants::OUTPUT_SIZE);

    auto new_vrf_value_begin =
        new_randomness.begin() + vrf_constants::OUTPUT_SIZE + 8;
    // NOLINTNEXTLINE
    for (size_t i = 0; i < last_epoch_vrf_values_.size(); ++i) {
      auto const &value_bytes = last_epoch_vrf_values_[i];
      std::copy(value_bytes.begin(), value_bytes.end(), new_vrf_value_begin);
      new_vrf_value_begin += 32;
    }
    last_epoch_vrf_values_.clear();

    return hasher_->blake2b_256(new_randomness);
  }

  void BabeLotteryImpl::submitVRFValue(const crypto::VRFPreOutput &value) {
    last_epoch_vrf_values_.push_back(value);
  }
}  // namespace kagome::consensus
